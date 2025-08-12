#ifndef SKY_FX
#define SKY_FX

cbuffer VPMatrices : register(b0)
{
    row_major float4x4 g_view_matrix;
    row_major float4x4 g_projection_matrix;
    row_major float4x4 g_vp_matrix;
};

TextureCube g_skybox_texture : register(t0);

SamplerState g_sampler : register(s0);

struct VS_INPUT
{
    uint vertex_id : SV_VertexID;
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
    
    float2 vertices[6] =
    {
        float2(-1.0f, 1.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, -1.0f),
        float2(-1.0f, 1.0f),
        float2(1.0f, -1.0f),
        float2(-1.0f, -1.0f)
    };
    
    output.position = float4(vertices[input.vertex_id], 0.0, 1.0f);
    output.position.z = output.position.w;
    
    float3 forward = float3(0.0f, 0.0f, 1.0f);
    forward = normalize(mul((float3x3) g_view_matrix, forward));
    
    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 right = normalize(cross(up, forward));
    
    up = normalize(cross(right, forward));
    
    output.texcoord = forward + vertices[input.vertex_id].x * right - vertices[input.vertex_id].y * up;
    output.texcoord = normalize(output.texcoord);
    
    return output;
}

PS_OUTPUT PSMain(VS_OUTPUT input)
{
    PS_OUTPUT output = (PS_OUTPUT) 0;
    
    output.color = g_skybox_texture.Sample(g_sampler, input.texcoord);
    
    return output;
}

#endif