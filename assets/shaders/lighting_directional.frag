#version 450
#extension GL_ARB_separate_shader_objects : enable

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

layout(input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput gbuffer_position;
layout(input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput gbuffer_normal;
layout(input_attachment_index = 2, set = 1, binding = 2) uniform subpassInput gbuffer_albedo_spec;

in VS_OUT {
    layout(location = 0) flat vec3 light_direction;
    layout(location = 1) flat vec3 light_color;
} fs_in;

layout(location = 0) out vec4 frag_color;

void main() {
    vec4 obj_position = subpassLoad(gbuffer_position);
    if (obj_position.a < 2.0) {
        discard;
    }

    vec3 obj_normal = subpassLoad(gbuffer_normal).xyz;
    vec3 albedo = subpassLoad(gbuffer_albedo_spec).rgb;
    float specular_value = subpassLoad(gbuffer_albedo_spec).a;

    vec3 light_dir = -fs_in.light_direction;
    vec3 view_dir = normalize(camera_constants.world_position - obj_position.xyz);

    vec3 ambient = 0.1 * albedo; // TODO: make it controllable

    vec3 diffuse = albedo * fs_in.light_color * max(dot(light_dir, obj_normal), 0.0);

    vec3 halfway_dir = normalize(light_dir + view_dir);
    vec3 specular = fs_in.light_color * (pow(max(dot(halfway_dir, obj_normal), 0.0), 32.0) * specular_value);

    frag_color = vec4((ambient + diffuse + specular), 1.0);
}