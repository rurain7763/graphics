#ifndef COMMON_GLSL
#define COMMON_GLSL

#define DIFFUSE_TEX_BINDING_FLAG (1 << 0)
#define SPECULAR_TEX_BINDING_FLAG (1 << 1)

bool has_texture(uint bindingFlags, uint textureFlags) {
    return (bindingFlags & textureFlags) == textureFlags;
}

vec2 get_texel_size(sampler2D texture) {
    ivec2 texture_size = textureSize(texture, 0);
    return vec2(1.0) / vec2(texture_size);
}

void calculate_phong_lighting(vec3 light_ambient, vec3 light_diffuse, vec3 light_specular, vec3 light_direction, vec3 view_direction, vec3 normal, vec3 diffuse_color, vec3 specular_color, float shininess, out vec3 ambient, out vec3 diffuse, out vec3 specular) {
    vec3 reflect_direction = reflect(light_direction, normal);
    
    ambient = light_ambient * diffuse_color;
    diffuse = light_diffuse * diffuse_color * max(dot(normal, -light_direction), 0.0);
    specular = light_specular * specular_color * pow(max(dot(-view_direction, reflect_direction), 0.0), shininess);
}

void calculate_blinn_phong_lighting(vec3 light_ambient, vec3 light_diffuse, vec3 light_specular, vec3 light_direction, vec3 view_direction, vec3 normal, vec3 diffuse_color, vec3 specular_color, float shininess, out vec3 ambient, out vec3 diffuse, out vec3 specular) {
    vec3 half_vector = normalize(-light_direction + -view_direction);

    ambient = light_ambient * diffuse_color;
    diffuse = light_diffuse * diffuse_color * max(dot(normal, -light_direction), 0.0);
    specular = light_specular * specular_color * pow(max(dot(normal, half_vector), 0.0), shininess);
}

float linearize_depth(float depth, float near, float far) {
    return near * far / (far + depth * (near - far));
}

vec3 gamma_correct(vec3 color, float gamma) {
    return pow(color, vec3(1.0 / gamma));
}

float calculate_shadow(vec4 light_space_position, sampler2D shadow_map) {
    vec3 proj_coords = light_space_position.xyz / light_space_position.w;
    proj_coords.xy = proj_coords.xy * 0.5 + 0.5;

    float shadow = 0.0;
    if (proj_coords.x >= 0.0 && proj_coords.x <= 1.0 && proj_coords.y >= 0.0 && proj_coords.y <= 1.0 && proj_coords.z <= 1.0) {
        vec2 texel_size = get_texel_size(shadow_map);
        for (int x = -1; x <= 1; ++x) {
            for (int y = -1; y <= 1; ++y) {
                vec2 offset = vec2(x, y) * texel_size;
                float pcf_depth = texture(shadow_map, proj_coords.xy + offset).r;
                float current_depth = proj_coords.z;
                shadow += current_depth > pcf_depth ? 1.0 : 0.0;
            }
        }
        shadow /= 9.0;
    }

    return shadow;
}

float calculate_shadow(vec3 view_position, vec3 frag_position, vec3 light_position, float far_plane, samplerCube shadow_map) {
    vec3 frag_to_light_dir = frag_position - light_position;
    float current_depth = length(frag_to_light_dir);

    float shadow = 0.0;
    if (current_depth <= far_plane) {
        frag_to_light_dir /= current_depth;

        vec3 sample_offsets[20] = vec3[] (
           vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
           vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
           vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
           vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
           vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
        );   

        int samples = 20;
        float view_dist = length(view_position - frag_position);
        float dist_radius = (1.0 + (view_dist / far_plane)) * 0.003;
        for(int i = 0; i < samples; ++i) {
            float closest_depth = texture(shadow_map, frag_to_light_dir + sample_offsets[i] * dist_radius).r;
            closest_depth *= far_plane;
            shadow += current_depth > closest_depth ? 1.0 : 0.0;
        }
        shadow /= float(samples);  
    }
    
    return shadow;
}

#endif