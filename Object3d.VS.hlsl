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
    output.position = input.position;
    return output;
    
    //D3D12_INPUT_ELEMENT_DESC inputElementDescs[1] = { };
    //inputElemntDescs[0].SemanticName = "POSITION";
    //inputElemntDescs[0].SemanticIndex = 0;
    //inputElemntDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    //inputElemntDescs[0].AlignedByteoffset = D3D12_APPEND_ALIGNED_ELEMENT;
    //D3D12_INPUT_LAYOUT_DESC inputLayouDesc{};
    //inputLayoutDesc.pInputElementDescs = inputElementDescs;
    //inputLayoutDesc.NumElements = _constof(inputElementDescs);            
}

