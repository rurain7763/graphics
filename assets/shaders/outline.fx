#ifndef OUTLINE_FX
#define OUTLINE_FX

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
    
    output.color = float4(1.0, 0.0, 0.0, 1.0);
    
    return output;
}

#endif