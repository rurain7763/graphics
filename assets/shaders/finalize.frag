#version 450
#extension GL_ARB_shading_language_include : enable

#include "common.glsl"

layout(set = 0, binding = 0) uniform sampler2D final_texture;
layout(set = 0, binding = 1) uniform sampler2D bloom_texture;

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 fragColor;

void main() {
    vec4 tex_color = texture(final_texture, in_tex_coord);
    vec4 bloom_color = texture(bloom_texture, in_tex_coord);
    vec4 final_color = vec4(0.0);

    final_color = tex_color;
    //final_color.rgb += bloom_color.rgb;

    // TODO: hdr exposure and gamma correction should be configurable
    final_color.rgb = exposure_tone_mapping(final_color.rgb, 1.0);
    final_color.rgb = gamma_correct(final_color.rgb, 2.2);

    fragColor = final_color;
}
