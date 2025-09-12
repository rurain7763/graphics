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
    Specular = (1 << 1),
	Normal = (1 << 2),
	Displacement = (1 << 3),
	AmbientOcclusion = (1 << 4)
};

struct MaterialConstants {
    vec3 diffuseColor;
    float shininess;
    float specular;
    uint32_t texture_binding_flags;
    uint32_t padding[2];
};

struct ObjectConstants {
    mat4 model_matrix;
    mat4 inv_model_matrix;
};

struct DirectionalLight {
    vec3 direction;
    vec3 color;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float linear;
    float quadratic;

    float GetDistance() const {
		const float threshold = 0.01f; // 원하는 최종 밝기 임계값
		float maxChannel = fmaxf(fmaxf(color.x, color.y), color.z);
		return (-linear + sqrtf(linear * linear - 4 * quadratic * (1.0f - maxChannel / threshold))) / (2.0f * quadratic);
    }
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
	vec3 tangent;
};

struct Material {
    vec3 diffuseColor;
    float specular;
    float shininess;

    Ref<Texture2D> diffuseTexture;
    Ref<Texture2D> specularTexture;
	Ref<Texture2D> normalTexture;
	Ref<Texture2D> displacementTexture;
	Ref<Texture2D> ambientOcclusionTexture;
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

struct GBuffer {
	Ref<Texture2D> position;
	Ref<Texture2D> normal;
	Ref<Texture2D> albedoSpec;
	Ref<Texture2D> ambientOcclusion;
};