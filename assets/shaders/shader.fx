#ifndef SHADER_FX
#define SHADER_FX

#include "common.fx"

struct DirectionalLight
{
    float3 direction;
    float padding;
    float3 ambient;
    float padding1;
    float3 diffuse;
    float padding2;
    float3 specular;
    float padding3;
};

struct PointLight
{
    float3 position;
    float constant_attenuation;
    float3 ambient;
    float linear_attenuation;
    float3 diffuse;
    float quadratic_attenuation;
    float3 specular;
    float padding;
};

struct SpotLight
{
    float3 position;
    float cutoff_inner_cosine;
    float3 direction;
    float cutoff_outer_cosine;
    float3 ambient;
    float constant_attenuation;
    float3 diffuse;
    float linear_attenuation;
    float3 specular;
    float quadratic_attenuation;
};

cbuffer CameraConstants : register(B_SET0_BINDING0)
{
    row_major float4x4 g_view_matrix;
    row_major float4x4 g_projection_matrix;
    row_major float4x4 g_vp_matrix;
    float3 g_camera_world_position;
    float g_camera_near_plane;
    float g_camera_far_plane;
    float g_camera_padding;
    float g_camera_padding1;
    float g_camera_padding2;
};

cbuffer LightingConstants : register(B_SET0_BINDING1)
{
    uint g_directional_light_count;
    uint g_point_light_count;
    uint g_spot_light_count;
    uint g_lighting_padding;
};

cbuffer MaterialConstants : register(B_SET1_BINDING1)
{
    float3 g_diffuse_color;
    float g_shininess;
    float3 g_specular_color;
    uint g_texture_binding_flags;
};

StructuredBuffer<DirectionalLight> g_directional_lights : register(T_SET0_BINDING4);
StructuredBuffer<PointLight> g_point_lights : register(T_SET0_BINDING5);
StructuredBuffer<SpotLight> g_spot_lights : register(T_SET0_BINDING6);

Texture2D g_diffuse_texture : register(T_SET1_BINDING2);
Texture2D g_specular_texture : register(T_SET1_BINDING3);
TextureCube g_skybox_texture : register(T_SET1_BINDING4);

SamplerState g_sampler : register(s0);

struct VS_INPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
    float4 model_matrix0 : MODEL_MATRIX;
    float4 model_matrix1 : MODEL_MATRIX1;
    float4 model_matrix2 : MODEL_MATRIX2;
    float4 model_matrix3 : MODEL_MATRIX3;
    float4 inv_model_matrix0 : INV_MODEL_MATRIX;
    float4 inv_model_matrix1 : INV_MODEL_MATRIX1;
    float4 inv_model_matrix2 : INV_MODEL_MATRIX2;
    float4 inv_model_matrix3 : INV_MODEL_MATRIX3;
    uint instance_id : SV_InstanceID;
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float3 world_position : POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR;
    float3 normal : NORMAL;
};

struct PS_OUTPUT
{
    float4 color : SV_Target0;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;

    float4x4 world_matrix = float4x4(input.model_matrix0, input.model_matrix1, input.model_matrix2, input.model_matrix3);
    float4x4 inv_world_matrix = float4x4(input.inv_model_matrix0, input.inv_model_matrix1, input.inv_model_matrix2, input.inv_model_matrix3);
    
    float4 world_position = mul(float4(input.position, 1.0f), world_matrix);
    float4 view_position = mul(world_position, g_view_matrix);
    float4 projected_position = mul(view_position, g_projection_matrix);
    
    output.position = projected_position;
    output.world_position = world_position.xyz;
    output.texcoord = input.texcoord;
    output.color = input.color;
    output.normal = normalize(mul((float3x3)transpose(inv_world_matrix), input.normal));
    
    return output;
}

PS_OUTPUT PSMain(VS_OUTPUT input)
{
    PS_OUTPUT output = (PS_OUTPUT) 0;
    
    float3 view_direction = normalize(g_camera_world_position - input.world_position);

    float3 diffuse_color = g_diffuse_color;
    if (has_texture(g_texture_binding_flags, DIFFUSE_TEX_BINDING_FLAG))
    {
        diffuse_color = g_diffuse_texture.Sample(g_sampler, input.texcoord).rgb;
    }

    float3 specular_color = g_specular_color;
    if (has_texture(g_texture_binding_flags, SPECULAR_TEX_BINDING_FLAG))
    {
        specular_color = g_specular_texture.Sample(g_sampler, input.texcoord).rgb;
    }

    float3 total_ambient = float3(0.0, 0.0, 0.0);
    float3 total_diffuse = float3(0.0, 0.0, 0.0);
    float3 total_specular = float3(0.0, 0.0, 0.0);

    for (uint i = 0; i < g_directional_light_count; ++i)
    {
        DirectionalLight light = g_directional_lights[i];

        float3 light_direction = light.direction;

        float3 ambient, diffuse, specular;
        calculate_blin_phong_lighting(light.ambient, light.diffuse, light.specular, light_direction, view_direction, input.normal, diffuse_color, specular_color, g_shininess, ambient, diffuse, specular);

        total_ambient += ambient;
        total_diffuse += diffuse;
        total_specular += specular;
    }

    for (uint i = 0; i < g_point_light_count; ++i)
    {
        PointLight light = g_point_lights[i];

        float3 light_direction = input.world_position - light.position;
        float distance = length(light_direction);
        light_direction = light_direction / distance;
        float attenuation = 1.0 / (light.constant_attenuation + light.linear_attenuation * distance + light.quadratic_attenuation * (distance * distance));

        float3 ambient, diffuse, specular;
        calculate_blin_phong_lighting(light.ambient, light.diffuse, light.specular, light_direction, view_direction, input.normal, diffuse_color, specular_color, g_shininess, ambient, diffuse, specular);

        total_ambient += ambient * attenuation;
        total_diffuse += diffuse * attenuation;
        total_specular += specular * attenuation;
    }

    for (uint i = 0; i < g_spot_light_count; ++i)
    {
        SpotLight light = g_spot_lights[i];

        float3 light_direction = input.world_position - light.position;
        float distance = length(light_direction);
        light_direction = light_direction / distance;
        float attenuation = 1.0 / (light.constant_attenuation + light.linear_attenuation * distance + light.quadratic_attenuation * (distance * distance));

        float spot_effect = dot(light.direction, light_direction);
        float epsilon = light.cutoff_inner_cosine - light.cutoff_outer_cosine;
        float intensity = clamp((spot_effect - light.cutoff_outer_cosine) / epsilon, 0.0, 1.0);

        float3 ambient, diffuse, specular;
        calculate_blin_phong_lighting(light.ambient, light.diffuse, light.specular, light_direction, view_direction, input.normal, diffuse_color, specular_color, g_shininess, ambient, diffuse, specular);

        total_ambient += ambient * attenuation * intensity;
        total_diffuse += diffuse * attenuation * intensity;
        total_specular += specular * attenuation * intensity;
    }

    output.color = float4(total_ambient + total_diffuse + total_specular, 1.0);
    
    return output;
}

#endif