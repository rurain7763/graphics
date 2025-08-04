#version 450

//layout(set = 0, binding = 2) uniform sampler2D textureSampler;

layout(std140, set = 0, binding = 1) uniform LightConstants {
    vec3 color;
    float intensity;
    vec3 direction;
    float padding;
} lightConstants;

layout(set = 1, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec2 in_tex_coord;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec4 fragColor;

void main() {
    float ambient_factor = 0.1;

    vec3 object_color = in_color * texture(textureSampler, in_tex_coord).rgb;
    vec3 light_color = lightConstants.color * lightConstants.intensity;
    vec3 ambient_color = object_color * ambient_factor * light_color;

    float diffuse = max(dot(in_normal, -lightConstants.direction), 0.0);

    fragColor = vec4(ambient_color + object_color * (diffuse * light_color), 1.0);
}
