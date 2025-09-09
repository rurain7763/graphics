#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

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

layout(set = 0, binding = 2) uniform GlobalConstants {
    float time;
    float delta_time;
    float padding0;
    float padding1;
} global_constants;

layout(set = 1, binding = 0) uniform ObjectConstants {
    mat4 model_matrix;
    mat4 inv_model_matrix;
} object_constants;

in VS_OUT {
    layout(location = 0) vec2 tex_coord;
    layout(location = 1) vec3 normal;
} gs_in[];

const float g_line_length = 0.1;

void main() {
    mat4 vp_matrix = camera_constants.projection_matrix * camera_constants.view_matrix;

    vec3 p0 = (object_constants.model_matrix * gl_in[0].gl_Position).xyz;
    vec3 p0_normal = normalize(mat3(transpose(object_constants.inv_model_matrix)) * gs_in[0].normal);
    vec3 p1 = (object_constants.model_matrix * gl_in[1].gl_Position).xyz;
    vec3 p1_normal = normalize(mat3(transpose(object_constants.inv_model_matrix)) * gs_in[1].normal);
    vec3 p2 = (object_constants.model_matrix * gl_in[2].gl_Position).xyz;
    vec3 p2_normal = normalize(mat3(transpose(object_constants.inv_model_matrix)) * gs_in[2].normal);

    gl_Position = vp_matrix * vec4(p0, 1.0);
    EmitVertex();

    gl_Position = vp_matrix * vec4(p0 + p0_normal * g_line_length, 1.0);
    EmitVertex();

    EndPrimitive();

    gl_Position = vp_matrix * vec4(p1, 1.0);
    EmitVertex();

    gl_Position = vp_matrix * vec4(p1 + p1_normal * g_line_length, 1.0);
    EmitVertex();

    EndPrimitive();

    gl_Position = vp_matrix * vec4(p2, 1.0);
    EmitVertex();

    gl_Position = vp_matrix * vec4(p2 + p2_normal * g_line_length, 1.0);
    EmitVertex();

    EndPrimitive();
}