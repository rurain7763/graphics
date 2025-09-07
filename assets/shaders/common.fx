#ifndef COMMON_GLSL
#define COMMON_GLSL

#define DIFFUSE_TEX_BINDING_FLAG (1 << 0)
#define SPECULAR_TEX_BINDING_FLAG (1 << 1)

bool has_texture(uint bindingFlags, uint textureFlags) {
    return (bindingFlags & textureFlags) == textureFlags;
}

float2 get_texel_size(Texture2D tex)
{
    uint width, height;
    tex.GetDimensions(width, height);
    return float2(1.0 / width, 1.0 / height);
}

void calculate_phong_lighting(float3 light_ambient, float3 light_diffuse, float3 light_specular, float3 light_direction, float3 view_direction, float3 normal, float3 diffuse_color, float3 specular_color, float shininess, out float3 ambient, out float3 diffuse, out float3 specular) {
    float3 reflect_dir = reflect(light_direction, normal);
    
    ambient = light_ambient * diffuse_color;
    diffuse = light_diffuse * diffuse_color * max(dot(normal, -light_direction), 0.0);
    specular = light_specular * specular_color * pow(max(dot(view_direction, reflect_dir), 0.0), shininess);
}

void calculate_blin_phong_lighting(float3 light_ambient, float3 light_diffuse, float3 light_specular, float3 light_direction, float3 view_direction, float3 normal, float3 diffuse_color, float3 specular_color, float shininess, out float3 ambient, out float3 diffuse, out float3 specular) {
    float3 half_vector = normalize(-light_direction + view_direction);
    
    ambient = light_ambient * diffuse_color;
    diffuse = light_diffuse * diffuse_color * max(dot(normal, -light_direction), 0.0);
    specular = light_specular * specular_color * pow(max(dot(normal, half_vector), 0.0), shininess);
}

float linearize_depth(float depth, float near, float far) {
    return near * far / (far + depth * (near - far));
}

float3 gamma_correct(float3 color, float gamma) {
    return pow(color, 1.0 / gamma);
}

float calculate_shadow(float4 light_space_position, Texture2D shadow_map, SamplerState s_state)
{
    float3 proj_coord = light_space_position.xyz / light_space_position.w;
    proj_coord.xy = proj_coord.xy * 0.5 + 0.5;
    proj_coord.y = 1.0 - proj_coord.y;
    
    float shadow = 0.0;
    if (proj_coord.x >= 0.0 && proj_coord.x <= 1.0 && proj_coord.y >= 0.0 && proj_coord.y <= 1.0 && proj_coord.z <= 1.0)
    {
        float2 texel_size = get_texel_size(shadow_map);
        for (int x = -1; x <= 1; ++x)
        {
            for (int y = -1; y <= 1; ++y)
            {
                float2 offset = float2(x, y) * texel_size;
                float closest_depth = shadow_map.Sample(s_state, proj_coord.xy + offset).r;
                float current_depth = proj_coord.z;
                shadow += current_depth > closest_depth ? 1.0 : 0.0;
            }
        }
        shadow /= 9.0;
    }
    
    return shadow;
}

#endif