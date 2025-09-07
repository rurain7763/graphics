#version 450
#extension GL_ARB_separate_shader_objects : enable

in VS_OUT {
    layout(location = 0) vec3 normal;
} fs_in;

void main() {
    vec3 light_dir = vec3(0.0, 0.0, 1.0); // TODO: Pass light direction from outside
    float bias = max(0.05 * (1.0 - dot(fs_in.normal, light_dir)), 0.005);
    
    gl_FragDepth = gl_FragCoord.z;
    gl_FragDepth += gl_FrontFacing ? bias : 0.0;
}