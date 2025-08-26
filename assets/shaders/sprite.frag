#version 450

layout(set = 1, binding = 1) uniform sampler2D diffuse_texture;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in vec3 in_normal;

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = texture(diffuse_texture, in_tex_coord);
}
