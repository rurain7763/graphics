#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBuffer {
    mat4 view_matrix;
    mat4 projection_matrix;
    mat4 view_projection_matrix;
} uniformBuffer;

layout(std140, set = 0, binding = 2) readonly buffer StorageBuffer {
    mat4 model_matrices[];
} storageBuffer;

/*
layout(push_constant) uniform PushConstants {
    mat4 model_matrix;
} pushConstants;
*/

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in vec3 in_normal;

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec2 out_tex_coord;
layout(location = 2) out vec3 out_normal;

void main() {
    gl_Position = uniformBuffer.view_projection_matrix * storageBuffer.model_matrices[gl_InstanceIndex] * vec4(in_position, 1.0);
    out_color = in_color;
    out_tex_coord = in_tex_coord;
    out_normal = normalize((storageBuffer.model_matrices[gl_InstanceIndex] * vec4(in_normal, 0.0)).xyz);
}