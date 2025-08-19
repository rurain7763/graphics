#version 450

//layout(set = 0, binding = 2) uniform sampler2D textureSampler;

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
    vec3 ambient_color;
    float padding;
    vec3 diffuse_color;
    float padding1;
    vec3 specular_color;
    float shininess;
} materialConstants;

layout(set = 1, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in vec3 in_normal;

layout(location = 0) out vec4 fragColor;

void main() {
    vec3 light_direction = normalize(in_position - lightConstants.position);
    vec3 view_direction = normalize(cameraConstants.world_position - in_position);
    vec3 reflect_direction = reflect(light_direction, in_normal);

    vec3 ambient_color = lightConstants.ambient * materialConstants.ambient_color;
    vec3 diffuse_color = lightConstants.diffuse * materialConstants.diffuse_color * max(dot(in_normal, -light_direction), 0.0);
    vec3 specular_color = lightConstants.specular * materialConstants.specular_color * pow(max(dot(view_direction, reflect_direction), 0.0), materialConstants.shininess);
    vec3 object_color = in_color * texture(textureSampler, in_tex_coord).rgb;

    fragColor = vec4((ambient_color + diffuse_color + specular_color) * object_color, 1.0);
}
