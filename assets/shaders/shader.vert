#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBuffer {
    mat4 view_matrix;
    mat4 projection_matrix;
    mat4 view_projection_matrix;
    vec3 world_position;
    float padding;
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

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec3 out_color;
layout(location = 2) out vec2 out_tex_coord;
layout(location = 3) out vec3 out_normal;

void main() {
    mat4 model_matrix = storageBuffer.model_matrices[gl_InstanceIndex];
    vec4 world_position = model_matrix * vec4(in_position, 1.0);

    gl_Position = uniformBuffer.projection_matrix * uniformBuffer.view_matrix * world_position;
    out_position = world_position.xyz;
    out_color = in_color;
    out_tex_coord = in_tex_coord;
    out_normal = normalize(mat3(transpose(inverse(model_matrix))) * in_normal);
}