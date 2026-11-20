#include "Object3d.hlsli"

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0; 
};

struct TransformtionMatrix
{
    float4x4 WVP;
    float4x4 world;
    float4x4 WorldInverseTranspose;
};
ConstantBuffer<TransformtionMatrix> gTransformationMatrix : register(b1); 

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;    
    
    output.position = mul(input.position,gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3)gTransformationMatrix.WorldInverseTranspose));
    output.worldPosition = mul(input.position, gTransformationMatrix.world).xyz;
    
    // 最後に結果を返す
    return output;
}

