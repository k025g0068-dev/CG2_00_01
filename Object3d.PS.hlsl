#include "Object3d.hlsli"
struct Material
{
    float32_t4 color;
};

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct pixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};


pixelShaderOutput main(VertexShaderOutput input)
{
    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    pixelShaderOutput output;
    output.color = gMaterial.color * textureColor;
    return output;
}
