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

layout(set = 1, binding = 0) uniform ObjectConstants {
    mat4 model_matrix;
    mat4 inv_model_matrix;
} object_constants;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in vec3 in_normal;
layout(location = 4) in vec3 in_tangent;

void main() {
    mat4 model_matrix = object_constants.model_matrix;
    mat4 inv_model_matrix = object_constants.inv_model_matrix;
    
    vec3 normal = normalize(mat3(transpose(inv_model_matrix)) * in_normal);
    vec4 world_position = model_matrix * vec4(in_position + 0.02 * normal, 1.0);

    gl_Position = camera_constants.projection_matrix * camera_constants.view_matrix * world_position;
}