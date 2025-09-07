#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform ShadowConstants {
    mat4 light_space_view[6];
	mat4 light_space_proj;
    vec3 light_position;
    float far_plane;
} shadow_constants;

in GS_OUT {
    layout(location = 0) vec3 normal;
    layout(location = 1) vec3 position;
} fs_in;

void main() {
    vec3 light_dir = fs_in.position - shadow_constants.light_position;
    float distance_to_frag = length(light_dir);
    light_dir /= distance_to_frag;

    float bias = max(0.05 * (1.0 - dot(fs_in.normal, light_dir)), 0.005);
    
    gl_FragDepth = distance_to_frag / shadow_constants.far_plane;
    gl_FragDepth += gl_FrontFacing ? bias : 0.0;
}