#version 450

#define DIFFUSE_TEX_BINDING_FLAG (1 << 0)
#define SPECULAR_TEX_BINDING_FLAG (1 << 1)

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

layout(set = 0, binding = 0) uniform CameraConstants {
    mat4 view_matrix;
    mat4 projection_matrix;
    mat4 view_projection_matrix;
    vec3 world_position;
    float near_plane;
    float far_plane;
    float padding0;
    float padding1;
    float padding2;
} camera_constants;

layout(std140, set = 0, binding = 1) uniform LightConstants {
    uint directional_light_count;
    uint point_light_count;
    uint spot_light_count;
    uint padding;
} lightConstants;

layout(std140, set = 1, binding = 1) uniform MaterialConstants {
    vec3 diffuse_color;
    float shininess;
    vec3 specular_color;
    uint texture_binding_flags;
} materialConstants;

layout(std140, set = 0, binding = 4) readonly buffer DirectionalLightBuffer {
    DirectionalLight data[];
} directional_lights;

layout(std140, set = 0, binding = 5) readonly buffer PointLightBuffer {
    PointLight data[];
} point_lights;

layout(std140, set = 0, binding = 6) readonly buffer SpotLightBuffer {
    SpotLight data[];
} spot_lights;

layout(set = 1, binding = 2) uniform sampler2D diffuse_texture;
layout(set = 1, binding = 3) uniform sampler2D specular_texture;
layout(set = 1, binding = 4) uniform samplerCube skybox_texture;

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(1.0, 1.0, 0.0, 1.0);
}
