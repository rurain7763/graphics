#version 450
#extension GL_ARB_shading_language_include : enable

#include "common.glsl"

layout(set = 0, binding = 0) uniform sampler2D bright_texture;

in VS_OUT {
    layout(location = 0) vec2 tex_coord;
} fs_in;

layout(location = 0) out vec4 frag_color;

void main() {
    vec4 color = texture(bright_texture, fs_in.tex_coord);

    float brightness = calc_brightness(color.rgb);
    if (brightness <= 1.0) {
        discard;
    }

    vec2 texel_size = get_texel_size(bright_texture);

    vec3 col0 = textureOffset(bright_texture, fs_in.tex_coord, ivec2(-2, 0)).rgb;
    vec3 col1 = textureOffset(bright_texture, fs_in.tex_coord, ivec2(2, 0)).rgb;
    vec3 col2 = textureOffset(bright_texture, fs_in.tex_coord, ivec2(0, -2)).rgb;
    vec3 col3 = textureOffset(bright_texture, fs_in.tex_coord, ivec2(0, 2)).rgb;

    frag_color = vec4((col0 + col1 + col2 + col3) * 0.25, 1.0);
}
