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
layout(location = 5) in mat4 in_instance_model_matrix;
layout(location = 9) in mat4 in_instance_inv_model_matrix;

out VS_OUT {
    layout(location = 0) vec3 position;
    layout(location = 1) vec4 color;
    layout(location = 2) vec2 tex_coord;
    layout(location = 3) vec3 normal;
    layout(location = 4) mat3 TBN_matrix;
} vs_out;

void main() {
    mat3 normal_matrix = mat3(transpose(in_instance_inv_model_matrix));
    
    vec3 N = normalize(normal_matrix * in_normal);
    vec3 T = normalize(normal_matrix * in_tangent);
    T = T - dot(T, N) * N;
    vec3 B = cross(N, T);

    vec4 world_position = in_instance_model_matrix * vec4(in_position, 1.0);

    gl_Position = camera_constants.projection_matrix * camera_constants.view_matrix * world_position;
    vs_out.position = world_position.xyz;
    vs_out.color = in_color;
    vs_out.tex_coord = in_tex_coord;
    vs_out.normal = N;
    vs_out.TBN_matrix = mat3(T, B, N);
}