#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in vec3 in_normal;
layout(location = 4) in vec3 in_tangent;

out VS_OUT {
    layout(location = 0) vec2 tex_coord;
} vs_out;

void main() {
    vec2 fullscreen;
    fullscreen.x = in_position.x / abs(in_position.x);
    fullscreen.y = in_position.y / abs(in_position.y);
    fullscreen.y *= -1.0;

    gl_Position = vec4(fullscreen, 0.0, 1.0);
    vs_out.tex_coord = in_tex_coord;
}