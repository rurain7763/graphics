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
} cameraConstants;

layout(std140, set = 0, binding = 1) uniform LightConstants {
    mat4 light_space_view;
    mat4 light_space_proj;
    uint directional_light_count;
    uint point_light_count;
    uint spot_light_count;
    float point_light_far_plane;
} lightConstants;

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
    layout(location = 4) vec4 light_space_position;
    layout(location = 5) mat3 TBN_matrix;
} vs_out;

void main() {
    mat4 model_matrix = in_instance_model_matrix;
    mat4 inv_model_matrix = in_instance_inv_model_matrix;
    mat3 normal_matrix = mat3(transpose(inv_model_matrix));
    
    vec3 N = normalize(normal_matrix * in_normal);
    vec3 T = normalize(normal_matrix * in_tangent);
    T = T - dot(T, N) * N;
    vec3 B = cross(N, T);

    vec4 world_position = model_matrix * vec4(in_position, 1.0);

    gl_Position = cameraConstants.projection_matrix * cameraConstants.view_matrix * world_position;
    vs_out.position = world_position.xyz;
    vs_out.color = in_color;
    vs_out.tex_coord = in_tex_coord;
    vs_out.normal = N;
    vs_out.light_space_position = lightConstants.light_space_proj * lightConstants.light_space_view * world_position;
    vs_out.TBN_matrix = mat3(T, B, N);
}