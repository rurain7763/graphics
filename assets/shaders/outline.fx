#ifndef OUTLINE_FX
#define OUTLINE_FX

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

cbuffer ObjectConstants : register(B_SET1_BINDING0)
{
    row_major float4x4 g_model_matrix;
    row_major float4x4 g_inv_model_matrix;
};

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
    float3 world_position : POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR;
    float3 normal : NORMAL;
};

struct PS_OUTPUT
{
    float4 color : SV_Target;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;
    
    float3 normal = normalize(mul((float3x3) transpose(g_inv_model_matrix), input.normal));
    
    float4 world_position = mul(float4(input.position + normal * 0.02, 1.0), g_model_matrix);
    float4 view_position = mul(world_position, g_view_matrix);
    float4 projected_position = mul(view_position, g_projection_matrix);

    output.position = projected_position;
    output.world_position = world_position.xyz;
    output.color = input.color;
    output.texcoord = input.texcoord;
    output.normal = normal;
    
    return output;
}

PS_OUTPUT PSMain(VS_OUTPUT input)
{
    PS_OUTPUT output = (PS_OUTPUT) 0;
    
    output.color = float4(1.0, 0.0, 0.0, 1.0);
    
    return output;
}

#endif