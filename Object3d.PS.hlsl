#include "Object3d.hlsli"


struct pixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    int32_t specularType;
    float32_t shininess;
    float32_t padding;
    float32_t4x4 uvTransform;
};

struct DirectionalLight
{
    float32_t4 color;
    float32_t3 direction;
    float intensity;
};

struct Camera
{
    float32_t3 worldPosition;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
ConstantBuffer<Camera> gCamera : register(b2);
Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);


pixelShaderOutput main(VertexShaderOutput input)
{
    pixelShaderOutput output;
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    if (textureColor.a == 0.0)
    {
        discard;
    }

    float3 N = normalize(input.normal);
    float3 L = normalize(-gDirectionalLight.direction);
    float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);

    if (gMaterial.enableLighting != 0)
    {
        float NdotL = dot(N, L);
        float cosTerm = pow(NdotL * 0.5f + 0.5f, 2.0f);

        float specularPow;
        if (gMaterial.specularType == 0)
        {
            // Phong
            float3 R = reflect(normalize(gDirectionalLight.direction), N);
            specularPow = pow(saturate(dot(R, toEye)), gMaterial.shininess);
        }
        else
        {
            // Blinn-Phong
            float3 H = normalize(L + toEye);
            specularPow = pow(saturate(dot(N, H)), gMaterial.shininess);
        }

        float3 diffuse = gMaterial.color.rgb * textureColor.rgb
                       * gDirectionalLight.color.rgb * cosTerm * gDirectionalLight.intensity;
        float3 specular = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularPow;

        output.color.rgb = diffuse + specular;
    }
    else
    {
        output.color.rgb = gMaterial.color.rgb * textureColor.rgb;
    }
    output.color.a = gMaterial.color.a * textureColor.a;
    return output;
}
