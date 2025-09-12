#version 450
#extension GL_ARB_separate_shader_objects : enable

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

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in vec3 in_normal;
layout(location = 4) in vec3 in_tangent;
layout(location = 5) in vec3 in_light_direction;
layout(location = 6) in vec3 in_light_color;

out VS_OUT {
    layout(location = 0) flat vec3 light_direction;
    layout(location = 1) flat vec3 light_color;
    layout(location = 2) vec2 tex_coord;
} vs_out;

void main() {
    vec2 fullscreen;
    fullscreen.x = in_position.x / abs(in_position.x);
    fullscreen.y = in_position.y / abs(in_position.y);
    fullscreen.y *= -1.0;

    gl_Position = vec4(fullscreen, 0.0, 1.0);
    vs_out.light_direction = in_light_direction;
    vs_out.light_color = in_light_color;
    vs_out.tex_coord = in_tex_coord;
}