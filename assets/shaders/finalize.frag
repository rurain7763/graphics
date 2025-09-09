#version 450
#extension GL_ARB_shading_language_include : enable

#include "common.glsl"

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput final_texture;

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 fragColor;

void main() {
    vec4 tex_color = subpassLoad(final_texture);
    vec4 final_color = vec4(0.0);

    final_color = tex_color;

    #if false
    // inversion
    final_color.rgb = vec4(vec3(1.0 - tex_color), 1.0);

    // gray scale
    float average = tex_color.r * 0.299 + tex_color.g * 0.587 + tex_color.b * 0.114;
    final_color.rgb = vec4(vec3(average), 1.0);

    // kernel
    vec2 texture_size = textureSize(final_texture, 0);
    vec2 texel_size = 1.0 / texture_size;

    vec2 offsets[9] = vec2[](
        vec2(-texel_size.x, -texel_size.y), // top-left
        vec2(0.0, -texel_size.y),           // top-center
        vec2(texel_size.x, -texel_size.y),  // top-right
        vec2(-texel_size.x, 0.0),           // center-left
        vec2(0.0, 0.0),                      // center
        vec2(texel_size.x, 0.0),            // center-right
        vec2(-texel_size.x, texel_size.y), // bottom-left
        vec2(0.0, texel_size.y),            // bottom-center
        vec2(texel_size.x, texel_size.y)   // bottom-right
    );

    float sharpen_kernel[9] = float[](
        -1, -1, -1,
        -1,  9, -1,
        -1, -1, -1
    );

    float blur_kernel[9] = float[](
        1.0 / 16, 2.0 / 16, 1.0 / 16,
        2.0 / 16, 4.0 / 16, 2.0 / 16,
        1.0 / 16, 2.0 / 16, 1.0 / 16
    );

    float edge_detection_kernel[9] = float[](
        -1, -1, -1,
        -1,  8, -1,
        -1, -1, -1
    );

    float kernel[9] = edge_detection_kernel;

    for (int i = 0; i < 9; i++) {
        final_color.rgb += texture(final_texture, in_tex_coord + offsets[i]).rgb * kernel[i];
    }
    #endif

    // TODO: hdr exposure and gamma correction should be configurable
    final_color.rgb = exposure_tone_mapping(final_color.rgb, 1.0);
    final_color.rgb = gamma_correct(final_color.rgb, 2.2);

    fragColor = final_color;
}
