#version 450

#define DIFFUSE_TEX_BINDING_FLAG (1 << 0)
#define SPECULAR_TEX_BINDING_FLAG (1 << 1)

layout(set = 0, binding = 0) uniform CameraConstants {
    mat4 view_matrix;
    mat4 projection_matrix;
    mat4 view_projection_matrix;
    vec3 world_position;
    float padding;
} cameraConstants;

layout(std140, set = 0, binding = 1) uniform LightConstants {
    vec3 position;
    float padding;
    vec3 ambient;
    float padding1;
    vec3 diffuse;
    float padding2;
    vec3 specular;
    float padding3;
} lightConstants;

layout(std140, set = 0, binding = 3) uniform MaterialConstants {
    uint texture_binding_flags;
    vec3 diffuse_color;
    vec3 specular_color;
    float shininess;
} materialConstants;

layout(set = 1, binding = 0) uniform sampler2D diffuse_texture;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in vec3 in_normal;

layout(location = 0) out vec4 fragColor;

bool has_texture(uint bindingFlags, uint textureFlags) {
    return (bindingFlags & textureFlags) == textureFlags;
}

void main() {
    vec3 light_direction = normalize(in_position - lightConstants.position);
    vec3 view_direction = normalize(cameraConstants.world_position - in_position);
    vec3 reflect_direction = reflect(light_direction, in_normal);

    vec3 diffuse_color = materialConstants.diffuse_color;
    if (has_texture(materialConstants.texture_binding_flags, DIFFUSE_TEX_BINDING_FLAG)) {
        diffuse_color = texture(diffuse_texture, in_tex_coord).rgb;
    }

    vec3 ambient = lightConstants.ambient * diffuse_color;
    vec3 diffuse = lightConstants.diffuse * diffuse_color * max(dot(in_normal, -light_direction), 0.0);
    vec3 specular = lightConstants.specular * materialConstants.specular_color * pow(max(dot(view_direction, reflect_direction), 0.0), materialConstants.shininess);

    fragColor = vec4(ambient + diffuse + specular, 1.0);
}
