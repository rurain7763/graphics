#version 450
#extension GL_ARB_shading_language_include : enable

layout(set = 0, binding = 0) uniform CameraConstants {
    mat4 view_matrix;
    mat4 projection_matrix;
    mat4 view_projection_matrix;
    vec3 world_position;
    float near_plane;
    float far_plane;
    float padding0;
    float padding1;
    float padding2;
} camera_constants;

layout(set = 0, binding = 1) uniform SSAOConstants {
    vec4 samples[64];
} ssao_constants;

layout(set = 0, binding = 2) uniform sampler2D gBuffer_position;
layout(set = 0, binding = 3) uniform sampler2D gBuffer_normal;
layout(set = 0, binding = 4) uniform sampler2D ssao_noise;

in VS_OUT {
    layout(location = 0) vec2 tex_coord;
} fs_in;

layout(location = 0) out float frag_color;

void main() {
    vec4 position = texture(gBuffer_position, fs_in.tex_coord);
    if (position.w < 2.0) {
        discard;
    }

    vec3 normal = texture(gBuffer_normal, fs_in.tex_coord).xyz;

    vec3 view_pos = (camera_constants.view_matrix * vec4(position.xyz, 1.0)).xyz;
    vec3 view_normal = normalize((camera_constants.view_matrix * vec4(normal, 0.0)).xyz);
    vec2 noise_scale = textureSize(gBuffer_position, 0).xy / textureSize(ssao_noise, 0).xy;
    vec3 random_vec = normalize(texture(ssao_noise, fs_in.tex_coord * noise_scale).xyz);

    vec3 tangent = normalize(random_vec - view_normal * dot(random_vec, view_normal));
    vec3 bitangent = cross(view_normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, view_normal);

    const float radius = 0.5;
    const float bias = 0.025;
    float occlusion = 0.0;
    for (int i = 0; i < 64; ++i) {
        vec3 sample_vec = TBN * ssao_constants.samples[i].xyz;
        sample_vec = view_pos + sample_vec * radius;

        vec4 offset = vec4(sample_vec, 1.0);
        offset = camera_constants.projection_matrix * offset;
        offset.xyz /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;

        float sample_depth = (camera_constants.view_matrix * vec4(texture(gBuffer_position, offset.xy).xyz, 1.0)).z;

        float range_check = smoothstep(0.0, 1.0, radius / abs(view_pos.z - sample_depth));
        occlusion += (sample_depth <= sample_vec.z + bias ? 1.0 : 0.0) * range_check;
    }

    occlusion = 1.0 - (occlusion / 64.0);

    frag_color = occlusion;
}
