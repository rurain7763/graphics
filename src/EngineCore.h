#pragma once

#include "Core.h"
#include "Math/Math.h"
#include "Graphics/GraphicsBuffers.h"
#include "Graphics/GraphicsTextures.h"
#include "Graphics/GraphicsHelper.h"

using namespace flaw;

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
    mat4 light_space_view_matrix;
	mat4 light_space_proj_matrix;
    uint32_t directional_light_count;
    uint32_t point_light_count;
    uint32_t spot_light_count;
	float point_light_far_plane;
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

struct TexturedVertex {
    vec3 position;
    vec4 color;
    vec2 texCoord;
    vec3 normal;
};

struct Material {
    vec3 diffuseColor;
    vec3 specularColor;
    float shininess;

    Ref<Texture2D> diffuseTexture;
    Ref<Texture2D> specularTexture;
};

struct MeshSegment {
    uint32_t vertexOffset;
    uint32_t indexOffset;
    uint32_t indexCount;
};

struct Mesh {
    Ref<VertexBuffer> vertexBuffer;
    Ref<IndexBuffer> indexBuffer;

    std::vector<MeshSegment> segments;
    std::vector<Ref<Material>> materials;
};

struct ShadowMap {
    mat4 lightSpaceView;
    mat4 lightSpaceProj;
    Ref<FramebufferGroup> framebufferGroup;
};

struct PointLightShadowMap {
	std::array<mat4, 6> lightSpaceViews;
	mat4 lightSpaceProj;
	vec3 lightPosition;
	float farPlane;
	Ref<FramebufferGroup> framebufferGroup;
};