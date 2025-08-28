#version 450

layout(set = 0, binding = 0) uniform sampler2D final_texture;

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = texture(final_texture, in_tex_coord);
}
