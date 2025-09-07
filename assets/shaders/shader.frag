#version 450
#extension GL_ARB_shading_language_include : enable

#include "common.glsl"

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
    mat4 light_space_view;
    mat4 light_space_proj;
    uint directional_light_count;
    uint point_light_count;
    uint spot_light_count;
    float point_light_far_plane;
} light_constants;

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
layout(set = 1, binding = 5) uniform sampler2D shadow_map_texture;
layout(set = 1, binding = 6) uniform samplerCube point_light_shadow_map_texture;

in VS_OUT {
    layout(location = 0) vec3 position;
    layout(location = 1) vec4 color;
    layout(location = 2) vec2 tex_coord;
    layout(location = 3) vec3 normal;
    layout(location = 4) vec4 light_space_position;
} fs_in;

layout(location = 0) out vec4 fragColor;

void main() {
    vec3 view_direction = normalize(fs_in.position - camera_constants.world_position);
    vec3 reflect_direction = reflect(view_direction, fs_in.normal);

    float refraction_ratio = 1.00 / 1.52; // Air to glass
    vec3 refract_direction = refract(view_direction, fs_in.normal, refraction_ratio);

    vec3 diffuse_color = materialConstants.diffuse_color;
    if (has_texture(materialConstants.texture_binding_flags, DIFFUSE_TEX_BINDING_FLAG)) {
        diffuse_color = texture(diffuse_texture, fs_in.tex_coord).rgb;
    }

    vec3 specular_color = materialConstants.specular_color;
    if (has_texture(materialConstants.texture_binding_flags, SPECULAR_TEX_BINDING_FLAG)) {
        specular_color = texture(specular_texture, fs_in.tex_coord).rgb;
    }

    vec3 total_ambient = vec3(0.0);
    vec3 total_diffuse = vec3(0.0);
    vec3 total_specular = vec3(0.0);

    for (uint i = 0; i < light_constants.directional_light_count; ++i) {
        DirectionalLight light = directional_lights.data[i];

        vec3 light_direction = light.direction;

        vec3 ambient, diffuse, specular;
        calculate_blinn_phong_lighting(light.ambient, light.diffuse, light.specular, light_direction, view_direction, fs_in.normal, diffuse_color, specular_color, materialConstants.shininess, ambient, diffuse, specular);

        total_ambient += ambient;
        total_diffuse += diffuse;
        total_specular += specular;
    }

    for (uint i = 0; i < light_constants.point_light_count; ++i) {
        PointLight light = point_lights.data[i];

        vec3 light_direction = fs_in.position - light.position;
        float distance = length(light_direction);
        light_direction = light_direction / distance;
        float attenuation = 1.0 / (light.constant_attenuation + light.linear_attenuation * distance + light.quadratic_attenuation * (distance * distance));

        vec3 ambient, diffuse, specular;
        calculate_blinn_phong_lighting(light.ambient, light.diffuse, light.specular, light_direction, view_direction, fs_in.normal, diffuse_color, specular_color, materialConstants.shininess, ambient, diffuse, specular);

        total_ambient += ambient * attenuation;
        total_diffuse += diffuse * attenuation;
        total_specular += specular * attenuation;
    }

    for (uint i = 0; i < light_constants.spot_light_count; ++i) {
        SpotLight light = spot_lights.data[i];

        vec3 light_direction = fs_in.position - light.position;
        float distance = length(light_direction);
        light_direction = light_direction / distance;
        float attenuation = 1.0 / (light.constant_attenuation + light.linear_attenuation * distance + light.quadratic_attenuation * (distance * distance));

        float spot_effect = dot(light.direction, light_direction);
        float epsilon = light.cutoff_inner_cosine - light.cutoff_outer_cosine;
        float intensity = clamp((spot_effect - light.cutoff_outer_cosine) / epsilon, 0.0, 1.0);

        vec3 ambient, diffuse, specular;
        calculate_blinn_phong_lighting(light.ambient, light.diffuse, light.specular, light_direction, view_direction, fs_in.normal, diffuse_color, specular_color, materialConstants.shininess, ambient, diffuse, specular);

        total_ambient += ambient * attenuation * intensity;
        total_diffuse += diffuse * attenuation * intensity;
        total_specular += specular * attenuation * intensity;
    }

    float shadow = calculate_shadow(fs_in.light_space_position, shadow_map_texture);
    shadow = calculate_shadow(camera_constants.world_position, fs_in.position, vec3(0.0), light_constants.point_light_far_plane, point_light_shadow_map_texture);
    vec3 object_color = total_ambient + (total_diffuse + total_specular) * (1.0 - shadow);
    vec3 environment_color = texture(skybox_texture, reflect_direction).rgb;
    vec3 refracted_color = texture(skybox_texture, refract_direction).rgb;

    vec3 final_color = mix(object_color, environment_color, 0.3);
    final_color = mix(final_color, refracted_color, 0.3);

    final_color = object_color;

    fragColor = vec4(final_color, 1.0);
}
