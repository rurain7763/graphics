#pragma once

#include "Platform/PlatformContext.h"
#include "Event/EventDispatcher.h"
#include "Graphics/GraphicsContext.h"
#include "Math/Math.h"
#include "Image/Image.h"

using namespace flaw;

#define USE_VULKAN 1
#define USE_DX11 0

struct CameraConstants {
    glm::mat4 view_matrix;
    glm::mat4 projection_matrix;
    glm::mat4 view_projection_matrix;

    glm::vec3 world_position;
	float padding;
};

struct LightConstants {
	glm::vec3 position;
    float padding;
    glm::vec3 ambient;
	float padding1;
	glm::vec3 diffuse;
	float padding2;
	glm::vec3 specular;
    float padding3;
};

struct MaterialConstants {
    glm::vec3 ambientColor;
    float padding;
    glm::vec3 diffuseColor;
	float padding1;
	glm::vec3 specularColor;
    float shininess;
};

struct TexturedVertex {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 texCoord;
    glm::vec3 normal;
};

struct Material {
	glm::vec3 ambientColor;
	glm::vec3 diffuseColor;
	glm::vec3 specularColor;
	float shininess;

    Ref<Texture2D> albedoTexture;
};

struct SubMesh {
    uint32_t vertexOffset;
    uint32_t indexOffset;
    uint32_t indexCount;
    uint32_t materialIndex;
};

struct Mesh {
    Ref<VertexBuffer> vertexBuffer;
    Ref<IndexBuffer> indexBuffer;

    std::vector<SubMesh> subMeshes;
    std::vector<Material> materials;
};

struct Object {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    Ref<Mesh> mesh;
};

extern Ref<PlatformContext> g_context;
extern EventDispatcher g_eventDispatcher;
extern Ref<GraphicsContext> g_graphicsContext;
extern std::vector<Object> g_objects;
extern std::unordered_map<std::string, Ref<Texture2D>> g_textures;
extern std::unordered_map<std::string, Ref<TextureCube>> g_textureCubes;
extern std::unordered_map<std::string, Ref<Mesh>> g_meshes;

void World_Init();
void World_Render();
void World_Cleanup();

Object& AddObject(const char* meshKey);

std::vector<uint8_t> GenerateTextureCubeData(Image& left, Image& right, Image& top, Image& bottom, Image& front, Image& back);