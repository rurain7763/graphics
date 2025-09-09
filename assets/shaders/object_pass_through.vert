#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in vec3 in_normal;
layout(location = 4) in vec3 in_tangent;

out VS_OUT {
    layout(location = 0) vec2 tex_coord;
    layout(location = 1) vec3 normal;
} vs_out;

void main() {
    gl_Position = vec4(in_position, 1.0);
    vs_out.tex_coord = in_tex_coord;
    vs_out.normal = in_normal;
}