struct TransformationMatrix
{
    float32_t4x4 WVP;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b1);

struct VertexShaderOutput
{
    float32_t4 position : SV_Position;
};

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
};


VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    output.position = mul(gTransformationMatrix.WVP,input.position);
    // 最後に結果を返す
    return output;
}

