#ifndef SPRITE_FX
#define SPRITE_FX

Texture2D g_diffuse_texture : register(t0);

SamplerState g_sampler : register(s0);

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

PS_OUTPUT PSMain(VS_OUTPUT input)
{
    PS_OUTPUT output = (PS_OUTPUT) 0;
    
    output.color = g_diffuse_texture.Sample(g_sampler, input.texcoord);
    
    return output;
}

#endif
