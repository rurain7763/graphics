#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

layout(set = 0, binding = 0) uniform ShadowConstants {
    mat4 light_space_view[6];
	mat4 light_space_proj;
    vec3 light_position;
    float far_plane;
} shadow_constants;

out GS_OUT {
    layout(location = 0) vec3 position;
} gs_out;

void main() {
    for (int face = 0; face < 6; ++face) {
        gl_Layer = face; // Select the cubemap face
        for (int i = 0; i < 3; ++i) {
            gs_out.position = gl_in[i].gl_Position.xyz;
            gl_Position = shadow_constants.light_space_proj * shadow_constants.light_space_view[face] * gl_in[i].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
    }
}