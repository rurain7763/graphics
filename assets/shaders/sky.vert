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

layout(location = 0) out vec3 out_tex_coord;

void main() {
    mat3 view_without_translation = mat3(camera_constants.view_matrix);
    vec4 view_position = vec4(view_without_translation * in_position, 1.0);

    gl_Position = camera_constants.projection_matrix * view_position;
    gl_Position.z = gl_Position.w;

    out_tex_coord = normalize(in_position);
}