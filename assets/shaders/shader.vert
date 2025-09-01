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

struct InstanceData {
    mat4 model_matrix;
    mat4 inv_model_matrix;
};

layout(std140, set = 1, binding = 0) readonly buffer InstanceDataBuffer {
    InstanceData data[];
} instance_datas;

/*
layout(push_constant) uniform PushConstants {
    mat4 model_matrix;
} pushConstants;
*/

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in vec3 in_normal;

out VS_OUT {
    layout(location = 0) vec3 position;
    layout(location = 1) vec4 color;
    layout(location = 2) vec2 tex_coord;
    layout(location = 3) vec3 normal;
} vs_out;

void main() {
    mat4 model_matrix = instance_datas.data[gl_InstanceIndex].model_matrix;
    mat4 inv_model_matrix = instance_datas.data[gl_InstanceIndex].inv_model_matrix;
    vec4 world_position = model_matrix * vec4(in_position, 1.0);

    gl_Position = cameraConstants.projection_matrix * cameraConstants.view_matrix * world_position;
    vs_out.position = world_position.xyz;
    vs_out.color = in_color;
    vs_out.tex_coord = in_tex_coord;
    vs_out.normal = normalize(mat3(transpose(inv_model_matrix)) * in_normal);
}