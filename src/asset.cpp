#include "pch.h"
#include "asset.h"
#include "world.h"
#include "Image/Image.h"
#include "Model/Model.h"
#include "Graphics/GraphicsFunc.h"
#include "Log/Log.h"

static std::unordered_map<std::string, Ref<Texture2D>> g_textures;
static std::unordered_map<std::string, Ref<TextureCube>> g_textureCubes;
static std::unordered_map<std::string, Ref<Mesh>> g_meshes;
static std::unordered_map<std::string, Ref<Material>> g_materials;

void Asset_Init() {
    Texture2D::Descriptor textureDesc;
    textureDesc.width = 1;
    textureDesc.height = 1;
    textureDesc.memProperty = MemoryProperty::Static;
    textureDesc.texUsages = TextureUsage::ShaderResource;
    textureDesc.format = PixelFormat::RGBA8Unorm;
    textureDesc.mipLevels = 1;
    textureDesc.initialLayout = TextureLayout::ShaderReadOnly;

    g_textures["dummy"] = g_graphicsContext->CreateTexture2D(textureDesc);

    Ref<Material> defaultMaterial = CreateRef<Material>();
    defaultMaterial->diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
    defaultMaterial->specular = 0.3f;
    defaultMaterial->shininess = 16.0f;

    g_materials["default"] = defaultMaterial;

    std::vector<TexturedVertex> quadVertices;
    std::vector<uint32_t> quadIndices;

    GenerateQuad([&](const glm::vec3& position, const glm::vec2& texCoord, const glm::vec3& normal, const glm::vec3& tangent, const glm::vec3& binormal) {
        TexturedVertex vertex;
        vertex.position = position;
        vertex.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex.texCoord = texCoord;
        vertex.normal = normal;
		vertex.tangent = tangent;
        quadVertices.push_back(vertex);
        }, quadIndices);

    std::vector<TexturedVertex> cubeVertices;
    std::vector<uint32_t> cubeIndices;
    GenerateCube(
        [&](const glm::vec3& position, const glm::vec2& texCoord, const glm::vec3& normal) {
            TexturedVertex vertex;
            vertex.position = position;
            vertex.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            vertex.texCoord = texCoord;
            vertex.normal = normal;
            cubeVertices.push_back(vertex);
        },
        cubeIndices
    );

	GenerateTangent(
		[&](uint32_t index, glm::vec3& pos, glm::vec2& uv) {
			pos = cubeVertices[index].position;
			uv = cubeVertices[index].texCoord;
		},
		cubeIndices,
		[&](uint32_t index, const glm::vec3& tangent) {
			cubeVertices[index].tangent = tangent;
		}
	);

    std::vector<TexturedVertex> sphereVertices;
    std::vector<uint32_t> sphereIndices;
    GenerateSphere(
        [&](const glm::vec3& position, const glm::vec2& texCoord, const glm::vec3& normal) {
            TexturedVertex vertex;
            vertex.position = position;
            vertex.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            vertex.texCoord = texCoord;
            vertex.normal = normal;
            sphereVertices.push_back(vertex);
        },
        sphereIndices,
        32,
        32,
        0.5f
    );

	GenerateTangent(
		[&](uint32_t index, glm::vec3& pos, glm::vec2& uv) {
			pos = sphereVertices[index].position;
			uv = sphereVertices[index].texCoord;
		},
		sphereIndices,
		[&](uint32_t index, const glm::vec3& tangent) {
			sphereVertices[index].tangent = tangent;
		}
	);

    LoadPrimitiveModel(quadVertices, quadIndices, "quad");
    LoadPrimitiveModel(cubeVertices, cubeIndices, "cube");
    LoadPrimitiveModel(sphereVertices, sphereIndices, "sphere");
}

void Asset_Cleanup() {
    g_meshes.clear();
    g_textureCubes.clear();
    g_textures.clear();
    g_materials.clear();
}

void LoadTexture(const char* filePath, PixelFormat pixelFormat, const char* key) {
    Image image(filePath, 4);

    if (!image.IsValid()) {
        Log::Error("Failed to load texture: %s", filePath);
        return;
    }

    Texture2D::Descriptor textureDesc;
    textureDesc.width = image.Width();
    textureDesc.height = image.Height();
    textureDesc.data = image.Data().data();
    textureDesc.memProperty = MemoryProperty::Static;
    textureDesc.texUsages = TextureUsage::ShaderResource;
    textureDesc.format = pixelFormat;
    textureDesc.mipLevels = GetMaxMipLevels(textureDesc.width, textureDesc.height);
	textureDesc.initialLayout = TextureLayout::ShaderReadOnly;

    g_textures[key] = g_graphicsContext->CreateTexture2D(textureDesc);
}

void LoadTextureCube(const std::array<const char*, 6>& faceFilePaths, const char* key) {
	std::array<Image, 6> images;
	for (size_t i = 0; i < 6; i++) {
		images[i] = Image(faceFilePaths[i], 4);
		if (!images[i].IsValid()) {
			Log::Error("Failed to load texture cube face: %s", faceFilePaths[i]);
			return;
		}
	}

	std::vector<uint8_t> textureData = GenerateTextureCubeData(images[0], images[1], images[2], images[3], images[4], images[5]);
	
    TextureCube::Descriptor textureDesc = {};
	textureDesc.width = images[0].Width();
	textureDesc.height = images[0].Height();
	textureDesc.data = textureData.data();
	textureDesc.format = PixelFormat::RGBA8Srgb;
	textureDesc.memProperty = MemoryProperty::Static;
	textureDesc.texUsages = TextureUsage::ShaderResource;
	textureDesc.initialLayout = TextureLayout::ShaderReadOnly;

	g_textureCubes[key] = g_graphicsContext->CreateTextureCube(textureDesc);
}

void LoadPrimitiveModel(const std::vector<TexturedVertex>& vertices, const std::vector<uint32_t>& indices, const char* key) {
    Ref<Mesh> mesh = CreateRef<Mesh>();

    VertexBuffer::Descriptor vertexBufferDesc;
    vertexBufferDesc.memProperty = MemoryProperty::Static;
    vertexBufferDesc.elmSize = sizeof(TexturedVertex);
    vertexBufferDesc.bufferSize = sizeof(TexturedVertex) * vertices.size();
    vertexBufferDesc.initialData = vertices.data();

    mesh->vertexBuffer = g_graphicsContext->CreateVertexBuffer(vertexBufferDesc);

    IndexBuffer::Descriptor indexBufferDesc;
    indexBufferDesc.memProperty = MemoryProperty::Static;
    indexBufferDesc.bufferSize = sizeof(uint32_t) * indices.size();
    indexBufferDesc.initialData = indices.data();

    mesh->indexBuffer = g_graphicsContext->CreateIndexBuffer(indexBufferDesc);

    MeshSegment subMesh;
    subMesh.vertexOffset = 0;
    subMesh.indexOffset = 0;
    subMesh.indexCount = indices.size();

    mesh->segments.push_back(subMesh);
    mesh->materials.push_back(g_materials["default"]);

    g_meshes[key] = mesh;
}

void LoadModel(const char* filePath, float scale, const char* key) {
    Ref<Mesh> mesh = CreateRef<Mesh>();
    Model model(filePath, scale);

    std::vector<TexturedVertex> vertices;
    for (const auto& vertex : model.GetVertices()) {
        TexturedVertex texturedVertex;
        texturedVertex.position = vertex.position;
        texturedVertex.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        texturedVertex.texCoord = vertex.texCoord;
        texturedVertex.normal = vertex.normal;
		texturedVertex.tangent = vertex.tangent;
        vertices.push_back(texturedVertex);
    }

    VertexBuffer::Descriptor vertexBufferDesc;
    vertexBufferDesc.memProperty = MemoryProperty::Static;
    vertexBufferDesc.elmSize = sizeof(TexturedVertex);
    vertexBufferDesc.bufferSize = sizeof(TexturedVertex) * vertices.size();
    vertexBufferDesc.initialData = vertices.data();

    mesh->vertexBuffer = g_graphicsContext->CreateVertexBuffer(vertexBufferDesc);

    const std::vector<uint32_t>& indices = model.GetIndices();

    IndexBuffer::Descriptor indexBufferDesc;
    indexBufferDesc.memProperty = MemoryProperty::Static;
    indexBufferDesc.bufferSize = sizeof(uint32_t) * indices.size();
    indexBufferDesc.initialData = indices.data();

    mesh->indexBuffer = g_graphicsContext->CreateIndexBuffer(indexBufferDesc);

    std::unordered_map<Ref<Image>, Ref<Texture2D>> textureCache;
    std::function<Ref<Texture2D>(const Ref<Image>&, PixelFormat)> createTexture = [&](const Ref<Image>& image, PixelFormat pixelFormat) {
        auto it = textureCache.find(image);
        if (it != textureCache.end()) {
            return it->second;
        }

        Texture2D::Descriptor textureDesc;
        textureDesc.width = image->Width();
        textureDesc.height = image->Height();
        textureDesc.data = image->Data().data();
        textureDesc.memProperty = MemoryProperty::Static;
        textureDesc.texUsages = TextureUsage::ShaderResource;
        textureDesc.format = pixelFormat;
        textureDesc.mipLevels = 1;
		textureDesc.initialLayout = TextureLayout::ShaderReadOnly;

        Ref<Texture2D> texture = g_graphicsContext->CreateTexture2D(textureDesc);
        textureCache[image] = texture;

        return texture;
    };

    std::unordered_map<uint32_t, Ref<Material>> materialCache;
    std::function<Ref<Material>(uint32_t, const ModelMaterial&)> createMaterial = [&](uint32_t index, const ModelMaterial& modelMaterial) {
        auto it = materialCache.find(index);
        if (it != materialCache.end()) {
            return it->second;
        }

        Ref<Material> material = CreateRef<Material>();
        material->diffuseColor = modelMaterial.baseColor;
        if (modelMaterial.diffuse) {
            material->diffuseTexture = createTexture(modelMaterial.diffuse, PixelFormat::RGBA8Srgb);
        }
        material->specular = 0.3f;
        if (modelMaterial.specular) {
            material->specularTexture = createTexture(modelMaterial.specular, PixelFormat::RGBA8Unorm);
        }
		if (modelMaterial.normal) {
			material->normalTexture = createTexture(modelMaterial.normal, PixelFormat::RGBA8Unorm);
		}
		if (modelMaterial.displacement) {
			material->displacementTexture = createTexture(modelMaterial.displacement, PixelFormat::RGBA8Unorm);
		}
        material->shininess = 32.0f;
        materialCache[index] = material;

        return material;
        };

    for (const auto& modelSubMesh : model.GetMeshs()) {
        MeshSegment subMesh;
        subMesh.vertexOffset = modelSubMesh.vertexStart;
        subMesh.indexOffset = modelSubMesh.indexStart;
        subMesh.indexCount = modelSubMesh.indexCount;
        mesh->segments.push_back(subMesh);

        Ref<Material> material;
        if (modelSubMesh.materialIndex == -1) {
            material = g_materials["default"];
        }
        else {
            material = createMaterial(modelSubMesh.materialIndex, model.GetMaterialAt(modelSubMesh.materialIndex));
        }

        mesh->materials.push_back(material);
    }

    g_meshes[key] = mesh;
}

void LoadMaterial(const char* key) {
	Ref<Material> material = CreateRef<Material>();
	material->diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
    material->specular = 0.3f;
	material->shininess = 16.0f;

	g_materials[key] = material;
}

Ref<Texture2D> GetTexture2D(const char* key) {
	auto it = g_textures.find(key);
	if (it != g_textures.end()) {
		return it->second;
	}
	Log::Error("Texture2D with key '%s' not found.", key);
	return nullptr;
}

Ref<TextureCube> GetTextureCube(const char* key) {
	auto it = g_textureCubes.find(key);
	if (it != g_textureCubes.end()) {
		return it->second;
	}
	Log::Error("TextureCube with key '%s' not found.", key);
	return nullptr;
}

Ref<Mesh> GetMesh(const char* key) {
	auto it = g_meshes.find(key);
	if (it != g_meshes.end()) {
		return it->second;
	}
	Log::Error("Mesh with key '%s' not found.", key);
	return nullptr;
}

Ref<Material> GetMaterial(const char* key) {
	auto it = g_materials.find(key);
	if (it != g_materials.end()) {
		return it->second;
	}
	Log::Error("Material with key '%s' not found.", key);
	return nullptr;
}