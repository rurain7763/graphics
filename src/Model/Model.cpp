#include "pch.h"
#include "Model.h"
#include "Log/Log.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/ProgressHandler.hpp>

namespace flaw {
	constexpr uint32_t BaseLoadOptFlags =
		aiProcess_Triangulate |
		aiProcess_GenNormals |
		aiProcess_CalcTangentSpace |
		aiProcess_JoinIdenticalVertices |
		aiProcess_LimitBoneWeights |
		aiProcess_OptimizeGraph |
		aiProcess_OptimizeMeshes |
		aiProcess_ValidateDataStructure |
		aiProcess_ImproveCacheLocality |
		aiProcess_FixInfacingNormals | 
		aiProcess_SortByPType;

	class CustomProgressHandler : public Assimp::ProgressHandler {
	public:
		CustomProgressHandler(const std::function<bool(float)>& progressHandler) : _progressHandler(progressHandler) {}

		bool Update(float pPercentage) override {
			return _progressHandler(pPercentage);
		}

	private:
		std::function<bool(float)> _progressHandler;
	};

	mat4 ToMat4(const aiMatrix4x4& mat) {
		return mat4(
			mat.a1, mat.b1, mat.c1, mat.d1,
			mat.a2, mat.b2, mat.c2, mat.d2,
			mat.a3, mat.b3, mat.c3, mat.d3,
			mat.a4, mat.b4, mat.c4, mat.d4
		);
	}

	uint32_t GetLoadOpFlags() {
		uint32_t loadOpFlags = BaseLoadOptFlags;
		
		if (ModelParams::LeftHanded) {
			loadOpFlags |= aiProcess_ConvertToLeftHanded;
		}

		return loadOpFlags;
	}

	Model::Model(const char* filePath, const std::function<bool(float)>& progressHandler) 
		: _loaded(false)
	{
		std::string extension = std::filesystem::path(filePath).extension().generic_string();

		_type = ModelType::Unknown;
		if (extension == ".obj") {
			_type = ModelType::Obj;
		}
		else if (extension == ".fbx") {
			_type = ModelType::Fbx;
		}

		Assimp::Importer importer;
		Scope<CustomProgressHandler> userProgressHandler;

		if (progressHandler) {
			userProgressHandler = CreateScope<CustomProgressHandler>(progressHandler);
			importer.SetProgressHandler(userProgressHandler.get());
		}

		const aiScene* scene = importer.ReadFile(filePath, GetLoadOpFlags());
		if (!scene || !scene->mRootNode) {
			Log::Error("[ASSIMP] %s", importer.GetErrorString());
			return;
		}

		if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE && scene->mAnimations == nullptr) {
			Log::Error("[ASSIMP] Incomplete scene: %s", importer.GetErrorString());
			return;
		}

		ParseScene(std::filesystem::path(filePath).parent_path(), scene);

		importer.SetProgressHandler(nullptr);

		_loaded = true;
	}

	Model::Model(ModelType type, const char* basePath, const char* memory, size_t size) 
		: _loaded(false)
	{
		_type = type;

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFileFromMemory(memory, size, GetLoadOpFlags());
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			Log::Error("ASSIMP: %s", importer.GetErrorString());
			return;
		}

		ParseScene(std::filesystem::path(basePath), scene);

		_loaded = true;
	}

	void Model::ParseScene(std::filesystem::path basePath, const aiScene* scene) {
		_globalInvMatrix = ToMat4(scene->mRootNode->mTransformation.Inverse());

		ParseSkeleton(_skeleton, scene->mRootNode, -1);

		_meshes.resize(scene->mNumMeshes);
		for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
			const aiMesh* mesh = scene->mMeshes[i];
			ParseMesh(scene, basePath, mesh, _meshes[i]);
		}

		_skeletalAnimations.resize(scene->mNumAnimations);
		for (uint32_t i = 0; i < scene->mNumAnimations; ++i) {
			const aiAnimation* animation = scene->mAnimations[i];
			ParseAnimation(scene, animation, _skeletalAnimations[i]);
		}
	}

	void Model::ParseSkeleton(ModelSkeleton& result, const aiNode* current, int32_t parentIndex) {
		int32_t nodeIndex = result.nodes.size();

		ModelSkeletonNode node;
		node.name = current->mName.data;
		node.parentIndex = parentIndex;
		node.transformMatrix = ToMat4(current->mTransformation);

		result.nodes.emplace_back(node);
		if (parentIndex != -1) {
			result.nodes[parentIndex].childrenIndices.push_back(nodeIndex);
		}

		for (uint32_t i = 0; i < current->mNumChildren; ++i) {
			ParseSkeleton(result, current->mChildren[i], nodeIndex);
		}
	}

	void Model::ParseMaterial(const aiScene* scene, const std::filesystem::path& basePath, const aiMaterial* aiMaterial, ModelMaterial& material) {	
		material.name = aiMaterial->GetName().C_Str();

		aiColor3D color(0.f, 0.f, 0.f);
		if (aiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
			material.baseColor = vec3(color.r, color.g, color.b);
		}
		
		aiString path;
		if (aiMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
			material.diffuse = GetImageOrCreate(scene, basePath / path.C_Str());
		}

		if (aiMaterial->GetTexture(aiTextureType_NORMALS, 0, &path) == AI_SUCCESS) {
			material.normal = GetImageOrCreate(scene, basePath / path.C_Str());
		}

		if (aiMaterial->GetTexture(aiTextureType_SPECULAR, 0, &path) == AI_SUCCESS) {
			material.specular = GetImageOrCreate(scene, basePath / path.C_Str());
		}

		if (aiMaterial->GetTexture(aiTextureType_SHININESS, 0, &path) == AI_SUCCESS) {
			material.shininess = GetImageOrCreate(scene, basePath / path.C_Str());
		}

		if (aiMaterial->GetTexture(aiTextureType_EMISSIVE, 0, &path) == AI_SUCCESS) {
			material.emissive = GetImageOrCreate(scene, basePath / path.C_Str());
		}

		if (aiMaterial->GetTexture(aiTextureType_HEIGHT, 0, &path) == AI_SUCCESS) {
			material.height = GetImageOrCreate(scene, basePath / path.C_Str());
		}

		if (aiMaterial->GetTexture(aiTextureType_BASE_COLOR, 0, &path) == AI_SUCCESS) {
			material.albedo = GetImageOrCreate(scene, basePath / path.C_Str());
		}

		if (aiMaterial->GetTexture(aiTextureType_OPACITY, 0, &path) == AI_SUCCESS) {
			material.opacity = GetImageOrCreate(scene, basePath / path.C_Str());
		}

		if (aiMaterial->GetTexture(aiTextureType_REFLECTION, 0, &path) == AI_SUCCESS) {
			material.reflection = GetImageOrCreate(scene, basePath / path.C_Str());
		}

		if (aiMaterial->GetTexture(aiTextureType_AMBIENT, 0, &path) == AI_SUCCESS) {
			material.ambient = GetImageOrCreate(scene, basePath / path.C_Str());
		}

		if (aiMaterial->GetTexture(aiTextureType_DISPLACEMENT, 0, &path) == AI_SUCCESS) {
			material.displacement = GetImageOrCreate(scene, basePath / path.C_Str());
		}

		if (aiMaterial->GetTexture(aiTextureType_LIGHTMAP, 0, &path) == AI_SUCCESS) {
			material.lightmap = GetImageOrCreate(scene, basePath / path.C_Str());
		}

		if (aiMaterial->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &path) == AI_SUCCESS) {
			material.ambientOcclusion = GetImageOrCreate(scene, basePath / path.C_Str());
		}

		if (aiMaterial->GetTexture(aiTextureType_METALNESS, 0, &path) == AI_SUCCESS) {
			material.metallic = GetImageOrCreate(scene, basePath / path.C_Str());
		}

		if (aiMaterial->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &path) == AI_SUCCESS) {
			material.roughness = GetImageOrCreate(scene, basePath / path.C_Str());
		}

		if (aiMaterial->GetTexture(aiTextureType_NORMAL_CAMERA, 0, &path) == AI_SUCCESS) {
			material.normal = GetImageOrCreate(scene, basePath / path.C_Str());
		}
	}

	Ref<Image> Model::GetImageOrCreate(const aiScene* scene, const std::filesystem::path& path) {
		auto it = _images.find(path);
		if (it != _images.end()) {
			return it->second;
		}

		Ref<Image> img;

		const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(path.generic_string().c_str());
		if (embeddedTexture) {
			img = CreateRef<Image>(Image::GetImageTypeFromExtension(path.generic_string().c_str()), (const char*)embeddedTexture->pcData, embeddedTexture->mWidth, 4);
		}
		else {
			img = CreateRef<Image>(path.generic_string().c_str(), 4);
		}

		if (img && img->IsValid()) {
			_images[path] = img;
		}

		return img;
	}

	void Model::ParseMesh(const aiScene* scene, const std::filesystem::path& basePath, const aiMesh* mesh, ModelMesh& modelMesh) {
		modelMesh.vertexStart = static_cast<uint32_t>(_vertices.size());
		modelMesh.vertexCount = mesh->mNumVertices;
		modelMesh.indexStart = static_cast<uint32_t>(_indices.size());
		modelMesh.indexCount = mesh->mNumFaces * 3;
		modelMesh.materialIndex = mesh->mMaterialIndex;

		_vertices.reserve(_vertices.size() + mesh->mNumVertices);
		_vertexBoneData.resize(_vertexBoneData.size() + mesh->mNumVertices);
		for (int32_t i = 0; i < mesh->mNumVertices; ++i) {
			ModelVertex vertex;
			vertex.position = vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
			vertex.normal = mesh->HasNormals() ? vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : vec3(0.0f);
			vertex.texCoord = mesh->HasTextureCoords(0) ? vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : vec2(0.0f);
			vertex.tangent = mesh->HasTangentsAndBitangents() ? vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z) : vec3(0.0f);
			vertex.bitangent = mesh->HasTangentsAndBitangents() ? vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z) : vec3(0.0f);

			_vertices.push_back(vertex);
		}

		_indices.reserve(_indices.size() + mesh->mNumFaces * 3);
		for (int32_t i = 0; i < mesh->mNumFaces; ++i) {
			const aiFace& face = mesh->mFaces[i];
			for (int32_t j = 0; j < face.mNumIndices; ++j) {
				_indices.push_back(face.mIndices[j]);
			}
		}

		if (_materials.find(mesh->mMaterialIndex) == _materials.end()) {
			const aiMaterial* aiMaterial = scene->mMaterials[mesh->mMaterialIndex];
			ParseMaterial(scene, basePath, aiMaterial, _materials[mesh->mMaterialIndex]);
		}

		if (mesh->HasBones()) {
			ParseBones(scene, mesh, modelMesh);
		}
	}

	void Model::ParseBones(const aiScene* scene, const aiMesh* mesh, ModelMesh& modelMesh) {
		for (int32_t i = 0; i < mesh->mNumBones; ++i) {
			const aiBone* bone = mesh->mBones[i];
			const std::string boneName = bone->mName.data;

			int32_t boneIndex = -1;

			auto it = _skeleton.boneMap.find(boneName);
			if (it == _skeleton.boneMap.end()) {
				int32_t nodeIndex = _skeleton.FindNode(boneName);
				boneIndex = _skeleton.boneMap.size();
				_skeleton.boneMap[boneName] = ModelSkeletonBoneNode{ nodeIndex, boneIndex, ToMat4(bone->mOffsetMatrix) };
			}
			else {
				boneIndex = it->second.boneIndex;
			}

			for (int32_t j = 0; j < bone->mNumWeights; ++j) {
				const aiVertexWeight& weight = bone->mWeights[j];

				ModelVertexBoneData& vertexBoneData = _vertexBoneData[modelMesh.vertexStart + weight.mVertexId];
				vertexBoneData.AddBoneWeight(boneIndex, weight.mWeight);
			}
		}
	}

	void Model::ParseAnimation(const aiScene* scene, const aiAnimation* animation, ModelSkeletalAnimation& skeletalAnim) {
		skeletalAnim.name = animation->mName.data;
		
		const float tickPerSecond = animation->mTicksPerSecond != 0 ? animation->mTicksPerSecond : 25.0f;
		skeletalAnim.durationSec = animation->mDuration / tickPerSecond;

		skeletalAnim._nodes.resize(animation->mNumChannels);
		for (int32_t i = 0; i < animation->mNumChannels; ++i) {
			const aiNodeAnim* channel = animation->mChannels[i];

			ModelSkeletalAnimationNode& boneAnim = skeletalAnim._nodes[i];
			boneAnim.name = channel->mNodeName.data;

			boneAnim.positionKeys.resize(channel->mNumPositionKeys);
			for (int32_t j = 0; j < channel->mNumPositionKeys; ++j) {
				const aiVectorKey& vecKey = channel->mPositionKeys[j];
				boneAnim.positionKeys[j] = { vecKey.mTime / tickPerSecond, vec3(vecKey.mValue.x, vecKey.mValue.y, vecKey.mValue.z) };
			}

			boneAnim.rotationKeys.resize(channel->mNumRotationKeys);
			for (int32_t j = 0; j < channel->mNumRotationKeys; ++j) {
				const aiQuatKey& quatKey = channel->mRotationKeys[j];
				boneAnim.rotationKeys[j] = { quatKey.mTime / tickPerSecond, vec4(quatKey.mValue.x, quatKey.mValue.y, quatKey.mValue.z, quatKey.mValue.w) };
			}

			boneAnim.scaleKeys.resize(channel->mNumScalingKeys);
			for (int32_t j = 0; j < channel->mNumScalingKeys; ++j) {
				const aiVectorKey& vecKey = channel->mScalingKeys[j];
				boneAnim.scaleKeys[j] = { vecKey.mTime / tickPerSecond, vec3(vecKey.mValue.x, vecKey.mValue.y, vecKey.mValue.z) };
			}
		}
	}
}