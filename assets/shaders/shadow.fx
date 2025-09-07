#ifndef SHADOW_FX
#define SHADOW_FX

cbuffer ShadowConstants : register(B_SET0_BINDING0)
{
    row_major float4x4 g_light_space_view;
    row_major float4x4 g_light_space_proj;
};

struct VS_INPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
    float4 model_matrix0 : MODEL_MATRIX;
    float4 model_matrix1 : MODEL_MATRIX1;
    float4 model_matrix2 : MODEL_MATRIX2;
    float4 model_matrix3 : MODEL_MATRIX3;
    float4 inv_model_matrix0 : INV_MODEL_MATRIX;
    float4 inv_model_matrix1 : INV_MODEL_MATRIX1;
    float4 inv_model_matrix2 : INV_MODEL_MATRIX2;
    float4 inv_model_matrix3 : INV_MODEL_MATRIX3;
};

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float3 normal : TEXCOORD0;
};

struct PS_INPUT
{
    float4 position : SV_Position;
    float3 normal : TEXCOORD0;
    bool is_front_face : SV_IsFrontFace;
};

struct PS_OUTPUT
{
    float depth : SV_Depth;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;
    
    float4x4 model_matrix = float4x4(
        input.model_matrix0,
        input.model_matrix1,
        input.model_matrix2,
        input.model_matrix3
    );
    
    float4x4 inv_model_matrix = float4x4(
        input.inv_model_matrix0,
        input.inv_model_matrix1,
        input.inv_model_matrix2,
        input.inv_model_matrix3
    );
    
    float4 world_position = mul(float4(input.position, 1.0f), model_matrix);
    float4 light_space_position = mul(world_position, g_light_space_view);
    float4 proj_position = mul(light_space_position, g_light_space_proj);
    
    output.position = proj_position;
    output.normal = normalize(mul(input.normal, (float3x3) transpose(inv_model_matrix)));
    
    return output;
}

PS_OUTPUT PSMain(PS_INPUT input)
{
    PS_OUTPUT output;
    
    float3 light_dir = float3(0.0, 0.0, 1.0); // TODO: Pass light direction from outside
    float bias = max(0.05 * (1.0 - dot(input.normal, light_dir)), 0.005);
    
    output.depth = input.position.z / input.position.w;
    output.depth += input.is_front_face ? bias : 0.0;
    
    return output;
}

#endif