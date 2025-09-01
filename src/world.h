#pragma once

#include "Platform/PlatformContext.h"
#include "Event/EventDispatcher.h"
#include "Graphics/GraphicsContext.h"
#include "Graphics/GraphicsHelper.h"
#include "Math/Math.h"
#include "Image/Image.h"
#include "EngineCamera.h"
#include "object.h"

using namespace flaw;

#define USE_VULKAN 1
#define USE_DX11 0

#define MAX_DIRECTIONAL_LIGHTS 1
#define MAX_POINT_LIGHTS 8
#define MAX_SPOT_LIGHTS 8

struct Material;

struct CameraConstants {
    mat4 view_matrix;
    mat4 projection_matrix;
    mat4 view_projection_matrix;
    vec3 world_position;
    float near_plane;
    float far_plane;
    float padding0;
    float padding1;
    float padding2;
};

struct LightConstants {
	uint32_t directional_light_count;
	uint32_t point_light_count;
	uint32_t spot_light_count;
	uint32_t padding;
};

struct GlobalConstants {
	float time;
	float delta_time;
	float padding0;
	float padding1;
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

struct ObjectConstants {
	mat4 model_matrix;
	mat4 inv_model_matrix;
};

struct InstanceData {
    glm::mat4 model_matrix;
	glm::mat4 inv_model_matrix;
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

extern Ref<PlatformContext> g_context;
extern EventDispatcher g_eventDispatcher;
extern Ref<GraphicsContext> g_graphicsContext;

extern Ref<EngineCamera> g_camera;
extern std::vector<Object> g_objects;

extern std::vector<Ref<Framebuffer>> g_sceneFramebuffers;
extern Ref<RenderPassLayout> g_sceneRenderPassLayout;
extern Ref<RenderPass> g_sceneClearRenderPass;
extern Ref<RenderPass> g_sceneLoadRenderPass;
extern Ref<VertexInputLayout> g_texturedVertexInputLayout;
extern Ref<ConstantBuffer> g_cameraCB;
extern Ref<ConstantBuffer> g_globalCB;
extern Ref<ConstantBuffer> g_lightCB;
extern Ref<StructuredBuffer> g_directionalLightSB;
extern Ref<StructuredBuffer> g_pointLightSB;
extern Ref<StructuredBuffer> g_spotLightSB;

extern Ref<ConstantBufferPool> g_objMaterialCBPool;
extern Ref<ConstantBufferPool> g_objConstantsCBPool;

void World_Init();
void World_Cleanup();
void World_Update();
void World_Render();
void World_FinalizeRender();

void Skybox_Init();
void Skybox_Cleanup();
void Skybox_Render();

void Geometry_Init();
void Geometry_Cleanup();
void Geometry_Render();

Object& AddObject();
Object& GetObjectWithName(const char* name);

MaterialConstants GetMaterialConstants(Ref<Material> material);

std::vector<uint8_t> GenerateTextureCubeData(Image& left, Image& right, Image& top, Image& bottom, Image& front, Image& back);