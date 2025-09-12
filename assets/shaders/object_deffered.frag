#version 450
#extension GL_ARB_shading_language_include : enable

#include "common.glsl"

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

layout(std140, set = 1, binding = 0) uniform MaterialConstants {
    vec3 diffuse_color;
    float shininess;
    float specular;
    uint texture_binding_flags;
    uint padding0;
    uint padding1;
} material_contstants;

layout(set = 1, binding = 1) uniform sampler2D diffuse_texture;
layout(set = 1, binding = 2) uniform sampler2D specular_texture;
layout(set = 1, binding = 3) uniform sampler2D normal_texture;
layout(set = 1, binding = 4) uniform sampler2D displacement_texture;
layout(set = 1, binding = 5) uniform sampler2D ao_texture;

in VS_OUT {
    layout(location = 0) vec3 position;
    layout(location = 1) vec4 color;
    layout(location = 2) vec2 tex_coord;
    layout(location = 3) vec3 normal;
    layout(location = 4) mat3 TBN_matrix;
} fs_in;

layout(location = 0) out vec4 object_position;
layout(location = 1) out vec4 object_normal;
layout(location = 2) out vec4 object_albedo_spec;
layout(location = 3) out float object_ao;

void main() {
    vec2 texcoord = fs_in.tex_coord;
    if (has_texture(material_contstants.texture_binding_flags, DISPLACEMENT_TEX_BINDING_FLAG)) {
        vec3 frag_to_view_in_TBN = camera_constants.world_position - fs_in.position;
        frag_to_view_in_TBN = normalize(fs_in.TBN_matrix * frag_to_view_in_TBN);

        float height_scale = 0.1; // TODO: Make configurable
        texcoord = parallax_mapping(texcoord, frag_to_view_in_TBN, height_scale, displacement_texture);
    }

    vec3 normal = fs_in.normal;
    if (has_texture(material_contstants.texture_binding_flags, NORMAL_TEX_BINDING_FLAG)) {
        normal = texture(normal_texture, texcoord).rgb;
        normal = normal * 2.0 - 1.0;
        normal = normalize(fs_in.TBN_matrix * normal);
    }

    vec3 diffuse_color = material_contstants.diffuse_color;
    if (has_texture(material_contstants.texture_binding_flags, DIFFUSE_TEX_BINDING_FLAG)) {
        diffuse_color = texture(diffuse_texture, texcoord).rgb;
    }

    float specular = material_contstants.specular;
    if (has_texture(material_contstants.texture_binding_flags, SPECULAR_TEX_BINDING_FLAG)) {
        specular = texture(specular_texture, texcoord).r;
    }

    float ao = 1.0;
    if (has_texture(material_contstants.texture_binding_flags, AO_TEX_BINDING_FLAG)) {
        ao = texture(ao_texture, texcoord).r;
    }

    object_position = vec4(fs_in.position, 2.0);
    object_normal = vec4(normal, 0.0);
    object_albedo_spec = vec4(diffuse_color, specular);
    object_ao = ao;
}
