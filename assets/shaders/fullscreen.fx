#ifndef FULLSCREEN_FX
#define FULLSCREEN_FX

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
    float2 texcoord : TEXCOORD0;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;
    
    float2 fullscreen;
    fullscreen.x = input.position.x / abs(input.position.x);
    fullscreen.y = input.position.y / abs(input.position.y);

    output.position = float4(fullscreen, 0.0, 1.0);
    output.texcoord = input.texcoord;
    
    return output;
}

#endif