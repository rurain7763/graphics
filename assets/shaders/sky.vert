#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBuffer {
    mat4 view_matrix;
    mat4 projection_matrix;
    mat4 view_projection_matrix;
} uniformBuffer;

vec2 screen_positions[6] = vec2[](
    vec2(-1.0, -1.0), 
    vec2(-1.0, 1.0), 
    vec2(1.0, 1.0),
    vec2(1.0, 1.0), 
    vec2(1.0, -1.0), 
    vec2(-1.0, -1.0)
);

layout(location = 0) out vec3 out_tex_coord;

void main() {
    vec3 viewed_position = mat3(uniformBuffer.view_matrix) * screen_positions[gl_VertexIndex];
    vec4 projected_position = uniformBuffer.projection_matrix * vec4(viewed_position, 1.0);

    gl_Position = projected_position;
    gl_Position.z = gl_Position.w;
    out_tex_coord = normalize(gl_Position.xyz);
}