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
};

struct LightConstants {
    glm::vec3 color;
    float intensity;
    glm::vec3 direction;
    float padding;
};

struct TexturedVertex {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 texCoord;
    glm::vec3 normal;
};

struct Material {
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
extern std::unordered_map<std::string, Ref<Material>> g_materials;

void World_Init();
void World_Update();
void World_Render();
void World_Cleanup();

void AddObject(const char* meshKey);

std::vector<uint8_t> GenerateTextureCubeData(Image& left, Image& right, Image& top, Image& bottom, Image& front, Image& back);