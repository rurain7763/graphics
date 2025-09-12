#version 450
#extension GL_ARB_shading_language_include : enable

#include "common.glsl"

layout(set = 0, binding = 0) uniform sampler2D ssao_input;

in VS_OUT {
    layout(location = 0) vec2 tex_coord;
} fs_in;

layout(location = 0) out float frag_color;

void main() {
    vec2 texel_size = get_texel_size(ssao_input);

    float result = 0.0;
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texel_size;
            result += texture(ssao_input, fs_in.tex_coord + offset).r;
        }
    }

    frag_color = result / 16.0;
}
