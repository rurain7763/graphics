#pragma once

#include "Platform/PlatformContext.h"
#include "Event/EventDispatcher.h"
#include "Graphics/GraphicsContext.h"
#include "Math/Math.h"
#include "Image/Image.h"

using namespace flaw;

#define USE_VULKAN 1
#define USE_DX11 0

#define MAX_DIRECTIONAL_LIGHTS 1
#define MAX_POINT_LIGHTS 8
#define MAX_SPOT_LIGHTS 8

struct CameraConstants {
    glm::mat4 view_matrix;
    glm::mat4 projection_matrix;
    glm::mat4 view_projection_matrix;

    glm::vec3 world_position;
	float padding;
};

struct LightConstants {
	uint32_t directional_light_count;
	uint32_t point_light_count;
	uint32_t spot_light_count;
	uint32_t padding;
};

enum TextureBindingFlag {
    Diffuse = (1 << 0),
    Specular = (1 << 1)
};

struct MaterialConstants {
    glm::vec3 diffuseColor;
    float shininess;
	glm::vec3 specularColor;
    uint32_t texture_binding_flags;
};

struct TexturedVertex {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 texCoord;
    glm::vec3 normal;
};

struct DirectionalLight {
    vec3 direction;
    float padding;
    vec3 ambient;
    float padding1;
    vec3 diffuse;
    float padding2;
    vec3 specular;
    float padding3;
};

struct PointLight {
    vec3 position;
    float constant_attenuation;
    vec3 ambient;
    float linear_attenuation;
    vec3 diffuse;
    float quadratic_attenuation;
    vec3 specular;
    float padding;
};

struct SpotLight {
    vec3 position;
    float cutoff_inner_cosine;
    vec3 direction;
    float cutoff_outer_cosine;
    vec3 ambient;
    float constant_attenuation;
    vec3 diffuse;
    float linear_attenuation;
    vec3 specular;
    float quadratic_attenuation;
};

struct Material {
	glm::vec3 diffuseColor;
	glm::vec3 specularColor;
	float shininess;

    Ref<Texture2D> diffuseTexture;
	Ref<Texture2D> specularTexture;
};

struct SubMesh {
    uint32_t vertexOffset;
    uint32_t indexOffset;
    uint32_t indexCount;
};

struct Mesh {
    Ref<VertexBuffer> vertexBuffer;
    Ref<IndexBuffer> indexBuffer;

    std::vector<SubMesh> subMeshes;
    std::vector<Ref<Material>> materials;
};

struct Object {
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
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
void World_Render();
void World_Cleanup();

Object& AddObject();

std::vector<uint8_t> GenerateTextureCubeData(Image& left, Image& right, Image& top, Image& bottom, Image& front, Image& back);