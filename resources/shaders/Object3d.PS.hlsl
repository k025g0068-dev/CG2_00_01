#include "Object3d.hlsli"


struct pixelShaderOutput
{
    float4 color : SV_TARGET0;
};

struct Material
{
    float4 color;
    int enableLighting;
    int specularType;
    float shininess;
    float padding;
    float4x4 uvTransform;
};

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

struct Camera
{
    float3 worldPosition;
};

struct PointLight
{
    float4 color;
    float3 position;
    float intensity;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
ConstantBuffer<Camera> gCamera : register(b2);
ConstantBuffer<PointLight> gPointLight : register(b3);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

float CalcSpecularPow(float3 lightDir, float3 N, float3 toEye)
{
    if (gMaterial.specularType == 0)
    {
        float3 R = reflect(lightDir, N); // Phong
        return pow(saturate(dot(R, toEye)), gMaterial.shininess);
    }
    float3 H = normalize(-lightDir + toEye); // Blinn-Phong
    return pow(saturate(dot(N, H)), gMaterial.shininess);
}

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
        // DirectionalLight
        float cosTerm = pow(dot(N, L) * 0.5f + 0.5f, 2.0f);
        float specPowDir = CalcSpecularPow(normalize(gDirectionalLight.direction), N, toEye);

        float3 diffuseDirectionalLight = gMaterial.color.rgb * textureColor.rgb
            * gDirectionalLight.color.rgb * cosTerm * gDirectionalLight.intensity;
        float3 specularDirectionalLight = gDirectionalLight.color.rgb
            * gDirectionalLight.intensity * specPowDir;

        // PointLight
        float3 pointLightDirection = normalize(input.worldPosition - gPointLight.position);
        float cosTermPoint = pow(dot(N, -pointLightDirection) * 0.5f + 0.5f, 2.0f);
        float specPowPoint = CalcSpecularPow(pointLightDirection, N, toEye);
        float3 pointLightColor = gPointLight.color.rgb * gPointLight.intensity;

        float3 diffusePointLight = gMaterial.color.rgb * textureColor.rgb
            * pointLightColor * cosTermPoint;
        float3 specularPointLight = pointLightColor * specPowPoint;

        // 全部足す
        output.color.rgb = diffuseDirectionalLight + specularDirectionalLight
                         + diffusePointLight + specularPointLight;
    }
    else
    {
        output.color.rgb = gMaterial.color.rgb * textureColor.rgb;
    }
    output.color.a = gMaterial.color.a * textureColor.a;
    return output;
}
