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
layout(location = 9) in vec3 in_light_position;
layout(location = 10) in float in_light_radius;
layout(location = 11) in vec3 in_light_color;
layout(location = 12) in vec3 in_light_attenuation;

out VS_OUT {
    layout(location = 0) flat vec3 light_position;
    layout(location = 1) flat float light_radius;
    layout(location = 2) flat vec3 light_color;
    layout(location = 3) flat vec3 light_attenuation;
} vs_out;

void main() {
    gl_Position = camera_constants.projection_matrix * camera_constants.view_matrix * in_instance_model_matrix * vec4(in_position, 1.0);
    vs_out.light_position = in_light_position;
    vs_out.light_radius = in_light_radius;
    vs_out.light_color = in_light_color;
    vs_out.light_attenuation = in_light_attenuation;
}