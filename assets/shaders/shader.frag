#version 450

#define DIFFUSE_TEX_BINDING_FLAG (1 << 0)
#define SPECULAR_TEX_BINDING_FLAG (1 << 1)

#define DIRECT_LIGHTING 0
#define POINT_LIGHTING 0
#define SPOT_LIGHTING 1

layout(set = 0, binding = 0) uniform CameraConstants {
    mat4 view_matrix;
    mat4 projection_matrix;
    mat4 view_projection_matrix;
    vec3 world_position;
    float padding;
} cameraConstants;

layout(std140, set = 0, binding = 1) uniform LightConstants {
#if DIRECT_LIGHTING
    vec3 direction;
    float padding;
    vec3 ambient;
    float padding1;
    vec3 diffuse;
    float padding2;
    vec3 specular;
    float padding3;
#elif POINT_LIGHTING
    vec3 position;
    float constant_attenuation;
    vec3 ambient;
    float linear_attenuation;
    vec3 diffuse;
    float quadratic_attenuation;
    vec3 specular;
    float padding;
#elif SPOT_LIGHTING
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
#endif
} lightConstants;

layout(std140, set = 0, binding = 3) uniform MaterialConstants {
    vec3 diffuse_color;
    float shininess;
    vec3 specular_color;
    uint texture_binding_flags;
} materialConstants;

layout(set = 1, binding = 0) uniform sampler2D diffuse_texture;
layout(set = 1, binding = 1) uniform sampler2D specular_texture;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in vec3 in_normal;

layout(location = 0) out vec4 fragColor;

bool has_texture(uint bindingFlags, uint textureFlags) {
    return (bindingFlags & textureFlags) == textureFlags;
}

void calculate_phong_lighting(vec3 light_ambient, vec3 light_diffuse, vec3 light_specular, vec3 light_direction, vec3 view_direction, vec3 reflect_direction, vec3 diffuse_color, vec3 specular_color, float shininess, out vec3 ambient, out vec3 diffuse, out vec3 specular) {
    ambient = light_ambient * diffuse_color;
    diffuse = light_diffuse * diffuse_color * max(dot(in_normal, -light_direction), 0.0);
    specular = light_specular * specular_color * pow(max(dot(view_direction, reflect_direction), 0.0), shininess);
}

void main() {
#if DIRECT_LIGHTING
    vec3 light_direction = lightConstants.direction;
#elif POINT_LIGHTING
    vec3 light_direction = in_position - lightConstants.position;
    float distance = length(light_direction);
    light_direction = light_direction / distance;
    float attenuation = 1.0 / (lightConstants.constant_attenuation + lightConstants.linear_attenuation * distance + lightConstants.quadratic_attenuation * (distance * distance));
#elif SPOT_LIGHTING
    vec3 light_direction = in_position - lightConstants.position;
    float distance = length(light_direction);
    light_direction = light_direction / distance;
    float attenuation = 1.0 / (lightConstants.constant_attenuation + lightConstants.linear_attenuation * distance + lightConstants.quadratic_attenuation * (distance * distance));
#endif
    vec3 view_direction = normalize(cameraConstants.world_position - in_position);
    vec3 reflect_direction = reflect(light_direction, in_normal);

    vec3 diffuse_color = materialConstants.diffuse_color;
    if (has_texture(materialConstants.texture_binding_flags, DIFFUSE_TEX_BINDING_FLAG)) {
        diffuse_color = texture(diffuse_texture, in_tex_coord).rgb;
    }

    vec3 specular_color = materialConstants.specular_color;
    if (has_texture(materialConstants.texture_binding_flags, SPECULAR_TEX_BINDING_FLAG)) {
        specular_color = texture(specular_texture, in_tex_coord).rgb;
    }

    vec3 ambient = vec3(0.0);
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    calculate_phong_lighting(lightConstants.ambient, lightConstants.diffuse, lightConstants.specular, light_direction, view_direction, reflect_direction, diffuse_color, specular_color, materialConstants.shininess, ambient, diffuse, specular);

#if POINT_LIGHTING
    diffuse *= attenuation;
    specular *= attenuation;
#elif SPOT_LIGHTING
    float spot_effect = dot(lightConstants.direction, light_direction);
    float epsilon = lightConstants.cutoff_inner_cosine - lightConstants.cutoff_outer_cosine;
    float intensity = clamp((spot_effect - lightConstants.cutoff_outer_cosine) / epsilon, 0.0, 1.0);

    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
#endif

    fragColor = vec4(ambient + diffuse + specular, 1.0);
}
