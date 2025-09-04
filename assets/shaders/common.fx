#ifndef COMMON_GLSL
#define COMMON_GLSL

#define DIFFUSE_TEX_BINDING_FLAG (1 << 0)
#define SPECULAR_TEX_BINDING_FLAG (1 << 1)

bool has_texture(uint bindingFlags, uint textureFlags) {
    return (bindingFlags & textureFlags) == textureFlags;
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

#endif