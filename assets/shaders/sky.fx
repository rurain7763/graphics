#ifndef SKY_FX
#define SKY_FX

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

TextureCube g_skybox_texture : register(T_SET1_BINDING0);

SamplerState g_sampler : register(s0);

struct VS_INPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float3 texcoord : TEXCOORD0;
};

struct PS_OUTPUT
{
    float4 color : SV_Target0;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;
    
    float3x3 wiew_without_translation = (float3x3) g_view_matrix;
    float4 view_position = float4(mul(input.position, wiew_without_translation), 1.0);
    
    output.position = mul(view_position, g_projection_matrix);
    output.position.z = output.position.w;
    
    output.texcoord = normalize(input.position);
    
    return output;
}

PS_OUTPUT PSMain(VS_OUTPUT input)
{
    PS_OUTPUT output = (PS_OUTPUT) 0;
    
    output.color = g_skybox_texture.Sample(g_sampler, input.texcoord);
    
    return output;
}

#endif