#ifndef OBJECT_PASS_THROUGH_FX
#define OBJECT_PASS_THROUGH_FX

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
    float3 normal : NORMAL;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;
    
    output.position = float4(input.position, 1.0);
    output.texcoord = input.texcoord;
    output.normal = input.normal;
    
    return output;
}

#endif // OBJECT_PASS_THROUGH_FX
