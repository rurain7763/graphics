#version 450
#extension GL_ARB_separate_shader_objects : enable

void main() {
    vec3 light_dir = vec3(0.0, 0.0, 1.0); // TODO: Pass light direction from outside
    float bias = 0.001;
    
    gl_FragDepth = gl_FragCoord.z;
    gl_FragDepth += gl_FrontFacing ? bias : 0.0;
}