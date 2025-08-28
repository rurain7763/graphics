#ifndef FINALIZE_FX
#define FINALIZE_FX

Texture2D g_final_texture : register(t0);

SamplerState g_sampler : register(s0);

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

struct PS_OUTPUT
{
    float4 color : SV_Target0;
};

PS_OUTPUT PSMain(VS_OUTPUT input)
{
    PS_OUTPUT output = (PS_OUTPUT) 0;
    
    output.color = g_final_texture.Sample(g_sampler, input.texcoord);
    
    return output;
}

#endif
