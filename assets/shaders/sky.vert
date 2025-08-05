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
    gl_Position = vec4(screen_positions[gl_VertexIndex], 0.0, 1.0);

    vec3 forward = vec3(0.0, 0.0, 1.0);
    forward = normalize(mat3(uniformBuffer.view_matrix) * forward);

    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(forward, up));

    up = normalize(cross(right, forward));

    out_tex_coord = forward + screen_positions[gl_VertexIndex].x * right - screen_positions[gl_VertexIndex].y * up;
    out_tex_coord = normalize(out_tex_coord);
}