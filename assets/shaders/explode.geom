#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

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

layout(set = 0, binding = 2) uniform GlobalConstants {
    float time;
    float delta_time;
    float padding0;
    float padding1;
} global_constants;

layout(set = 1, binding = 0) uniform ObjectConstants {
    mat4 model_matrix;
    mat4 inv_model_matrix;
} object_constants;

in VS_OUT {
    layout(location = 0) vec2 tex_coord;
    layout(location = 1) vec3 normal;
} gs_in[];

out GS_OUT {
    layout(location = 0) vec3 position;
    layout(location = 1) vec4 color;
    layout(location = 2) vec2 tex_coord;
    layout(location = 3) vec3 normal;
} gs_out;

vec3 CalcNormal(vec3 p0, vec3 p1, vec3 p2) {
    vec3 u = p1 - p0;
    vec3 v = p2 - p0;
    return normalize(cross(u, v));
}

void main() {
    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz;
    vec3 normal = CalcNormal(p0, p1, p2);
    vec3 calc_normal = normalize(mat3(transpose(object_constants.inv_model_matrix)) * normal);

    float magnitude = 1.0;
    float factor = abs(sin(global_constants.time)) * magnitude;
    factor = mix(magnitude, 0.0, fract(global_constants.time));

    vec4 world_position0 = object_constants.model_matrix * vec4(p0 + normal * factor, 1.0);
    gl_Position = camera_constants.projection_matrix * camera_constants.view_matrix * world_position0;
    gs_out.position = world_position0.xyz;
    gs_out.color = vec4(1.0, 1.0, 1.0, 1.0);
    gs_out.tex_coord = gs_in[0].tex_coord;
    gs_out.normal = calc_normal;
    EmitVertex();

    vec4 world_position1 = object_constants.model_matrix * vec4(p1 + normal * factor, 1.0);
    gl_Position = camera_constants.projection_matrix * camera_constants.view_matrix * world_position1;
    gs_out.position = world_position1.xyz;
    gs_out.color = vec4(1.0, 1.0, 1.0, 1.0);
    gs_out.tex_coord = gs_in[1].tex_coord;
    gs_out.normal = calc_normal;
    EmitVertex();

    vec4 world_position2 = object_constants.model_matrix * vec4(p2 + normal * factor, 1.0);
    gl_Position = camera_constants.projection_matrix * camera_constants.view_matrix * world_position2;
    gs_out.position = world_position2.xyz;
    gs_out.color = vec4(1.0, 1.0, 1.0, 1.0);
    gs_out.tex_coord = gs_in[2].tex_coord;
    gs_out.normal = calc_normal;
    EmitVertex();

    EndPrimitive();
}