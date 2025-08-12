#ifndef SHADER_FX
#define SHADER_FX

cbuffer VPMatrices : register(b0)
{
    row_major float4x4 g_view_matrix;
    row_major float4x4 g_projection_matrix;
    row_major float4x4 g_vp_matrix;
};

cbuffer LightingConstants : register(b1)
{
    float3 g_lighting_color;
    float g_lighting_intensity;
    float3 g_lighting_direction;
    float g_lighting_padding;
};

struct BatchedData
{
    row_major float4x4 world_matrix;
};

StructuredBuffer<BatchedData> g_batched_data : register(t0);

Texture2D g_texture : register(t1);

SamplerState g_sampler : register(s0);

struct VS_INPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
    uint instance_id : SV_InstanceID;
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    float3 color : COLOR;
    float3 normal : NORMAL;
};

struct PS_OUTPUT
{
    float4 color : SV_Target0;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;
    
    float4 world_position = mul(float4(input.position, 1.0f), g_batched_data[input.instance_id].world_matrix);
    float4 view_position = mul(world_position, g_view_matrix);
    float4 projected_position = mul(view_position, g_projection_matrix);
    
    output.position = projected_position;
    output.texcoord = input.texcoord;
    output.color = input.color;
    output.normal = normalize(mul(float4(input.normal, 0.0f), g_batched_data[input.instance_id].world_matrix).xyz);
    
    return output;
}

PS_OUTPUT PSMain(VS_OUTPUT input)
{
    PS_OUTPUT output = (PS_OUTPUT) 0;
    
    float ambient_factor = 0.1;

    float3 object_color = input.color * g_texture.Sample(g_sampler, input.texcoord).rgb;
    float3 light_color = g_lighting_color * g_lighting_intensity;
    float3 ambient_color = object_color * ambient_factor * light_color;

    float diffuse = max(dot(input.normal, -g_lighting_direction), 0.0);

    output.color = float4(ambient_color + object_color * (diffuse * light_color), 1.0);
    
    return output;
}

#endif