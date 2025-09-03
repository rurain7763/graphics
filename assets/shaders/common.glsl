#ifndef COMMON_GLSL
#define COMMON_GLSL

#define DIFFUSE_TEX_BINDING_FLAG (1 << 0)
#define SPECULAR_TEX_BINDING_FLAG (1 << 1)

bool has_texture(uint bindingFlags, uint textureFlags) {
    return (bindingFlags & textureFlags) == textureFlags;
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

#endif