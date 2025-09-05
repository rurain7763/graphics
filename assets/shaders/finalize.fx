#ifndef FINALIZE_FX
#define FINALIZE_FX

#include "common.fx"

Texture2D g_final_texture : register(T_SET0_BINDING0);

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
    
    float4 final_color = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    final_color = g_final_texture.Sample(g_sampler, input.texcoord);
    
    final_color.rgb = gamma_correct(final_color.rgb, 2.2);
    
    output.color = final_color;
    
    return output;
}

#endif
