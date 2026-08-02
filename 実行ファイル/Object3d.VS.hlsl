#include "Object3d.hlsli"

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0; 
};

struct TransformtionMatrix
{
    float32_t4x4 WVP;
    float32_t4x4 world;
};
ConstantBuffer<TransformtionMatrix> gTransformationMatrix : register(b1); 

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;    
    
    output.position = mul(input.position,gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float32_t3x3)gTransformationMatrix.world));
    
    // 最後に結果を返す
    return output;
}

