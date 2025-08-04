#pragma once

#include "Core.h"
#include "Math/Math.h"
#include "Image/Image.h"

#include <map>
#include <filesystem>

struct aiScene;
struct aiMesh;
struct aiMaterial;
struct aiTexture;
struct aiSkeleton;
struct aiBone;
struct aiNode;
struct aiAnimation;
struct aiNodeAnim;

namespace flaw {
	struct ModelParams {
		constexpr static int32_t MaxInfluenceBoneCount = 4;
		inline static bool LeftHanded = true;
	};

	enum class ModelType {
		Obj,
		Fbx,
		Unknown
	};

	struct ModelVertex {
		vec3 position;
		vec3 normal;
		vec2 texCoord;
		vec3 tangent;
		vec3 bitangent;
	};

	struct ModelVertexBoneData {
		int32_t boneIndices[ModelParams::MaxInfluenceBoneCount] = { -1, -1, -1, -1 };
		float boneWeight[ModelParams::MaxInfluenceBoneCount] = { 0.0f, 0.0f, 0.0f, 0.0f };

		void AddBoneWeight(int32_t index, float weight) {
			for (int32_t i = 0; i < ModelParams::MaxInfluenceBoneCount; ++i) {
				if (boneIndices[i] == index) {
					return;
				}

				if (boneIndices[i] == -1) {
					boneIndices[i] = index;
					boneWeight[i] = weight;
					return;
				}
			}
		}
	};

	struct ModelMaterial {
		std::string name;

		vec3 baseColor = vec3(0.0f);

		Ref<Image> diffuse;
		Ref<Image> normal;
		Ref<Image> specular;
		Ref<Image> ambient;
		Ref<Image> emissive;
		Ref<Image> height;
		Ref<Image> shininess;
		Ref<Image> opacity;
		Ref<Image> displacement;
		Ref<Image> lightmap;
		Ref<Image> reflection;
		Ref<Image> albedo;
		Ref<Image> metallic;
		Ref<Image> roughness;
		Ref<Image> ambientOcclusion;
	};

	struct ModelSkeletonNode {
		std::string name;
		int32_t parentIndex = -1;
		mat4 transformMatrix = mat4(1.0f);
		std::vector<int32_t> childrenIndices;
	};

	struct ModelSkeletonBoneNode {
		int32_t nodeIndex = -1;
		int32_t boneIndex = -1;
		mat4 offsetMatrix = mat4(1.0f);
	};

	struct ModelSkeleton {
		std::vector<ModelSkeletonNode> nodes;
		std::unordered_map<std::string, ModelSkeletonBoneNode> boneMap;

		int32_t FindNode(const std::string& name) const {
			for (size_t i = 0; i < nodes.size(); ++i) {
				if (nodes[i].name == name) {
					return static_cast<int32_t>(i);
				}
			}

			throw std::runtime_error("Node not found: " + name);
		}
	};

	struct ModelSkeletalAnimationNode {
		std::string name;

		std::vector<std::pair<float, vec3>> positionKeys;
		std::vector<std::pair<float, vec4>> rotationKeys;
		std::vector<std::pair<float, vec3>> scaleKeys;
	};
	
	struct ModelSkeletalAnimation {
		std::string name;
		float durationSec = 0.0f;

		std::vector<ModelSkeletalAnimationNode> _nodes;
	};

	struct ModelMesh {
		uint32_t vertexStart = 0;
		uint32_t vertexCount = 0;
		uint32_t indexStart = 0;
		uint32_t indexCount = 0;
		int32_t materialIndex = -1;
	};

	class Model {
	public:
		Model() = default;
		Model(const char* filePath, const std::function<bool(float)>& progressHandler = nullptr);
		Model(ModelType type, const char* basePath, const char* memory, size_t size);

		ModelType GetModelType() const { return _type; }

		const ModelMaterial& GetMaterialAt(uint32_t index) const { return _materials.at(index); }
		const ModelSkeleton& GetSkeleton() const { return _skeleton; }
		const std::vector<ModelVertex>& GetVertices() const { return _vertices; }
		const ModelVertex& GetVertexAt(uint32_t index) const { return _vertices[index]; }
		const ModelVertexBoneData& GetVertexBoneDataAt(uint32_t index) const { return _vertexBoneData[index]; }
		const std::vector<uint32_t>& GetIndices() const { return _indices; }
		const std::vector<ModelMesh>& GetMeshs() const { return _meshes; }
		const std::vector<ModelSkeletalAnimation>& GetSkeletalAnimations() const { return _skeletalAnimations; }

		const mat4& GetGlobalInvMatrix() const { return _globalInvMatrix; }

		bool IsStaticModel() const { return !_meshes.empty() && _skeleton.boneMap.empty(); }
		bool HasMeshes() const { return !_meshes.empty(); }

		bool IsValid() const { return _loaded; }

	private:
		void ParseScene(std::filesystem::path basePath, const aiScene* scene);
		void ParseSkeleton(ModelSkeleton& result, const aiNode* current, int32_t parentIndex);
		void ParseMesh(const aiScene* scene, const std::filesystem::path& basePath, const aiMesh* mesh, ModelMesh& modelMesh);
		void ParseMaterial(const aiScene* scene, const std::filesystem::path& basePath, const aiMaterial* aiMaterial, ModelMaterial& material);
		void ParseBones(const aiScene* scene, const aiMesh* mesh, ModelMesh& modelMesh);
		void ParseAnimation(const aiScene* scene, const aiAnimation* animation, ModelSkeletalAnimation& skeletalAnim);

		Ref<Image> GetImageOrCreate(const aiScene* scene, const std::filesystem::path& path);

	private: 
		ModelType _type = ModelType::Unknown;
		bool _loaded;

		mat4 _globalInvMatrix = mat4(1.0f);

		std::unordered_map<std::filesystem::path, Ref<Image>> _images;
		std::unordered_map<uint32_t, ModelMaterial> _materials;

		std::vector<ModelVertex> _vertices;
		std::vector<ModelVertexBoneData> _vertexBoneData;
		std::vector<uint32_t> _indices;

		ModelSkeleton _skeleton;
		std::vector<ModelSkeletalAnimation> _skeletalAnimations;

		std::vector<ModelMesh> _meshes;
	};
}