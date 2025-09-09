#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec2 in_tex_coord;
layout(location = 3) in vec3 in_normal;
layout(location = 4) in vec3 in_tangent;
layout(location = 5) in mat4 in_instance_model_matrix;
layout(location = 9) in mat4 in_instance_inv_model_matrix;

void main() {
    vec4 world_position = in_instance_model_matrix * vec4(in_position, 1.0);

    gl_Position = world_position;
}