#version 450

layout(set = 1, binding = 0) uniform samplerCube skybox_sampler;

layout(location = 0) in vec3 in_tex_coord;

layout(location = 0) out vec4 frag_color;

void main() {
    frag_color = texture(skybox_sampler, in_tex_coord);
}
