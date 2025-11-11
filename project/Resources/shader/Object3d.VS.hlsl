#include "Object3d.hlsli"

struct TransformaitionMatrix
{
    float32_t4x4 WVP;
    float32_t4x4 World;
    float32_t4x4 WorldInverseTranspose;
};
ConstantBuffer<TransformaitionMatrix> gTransformaitionMatrix : register(b0);

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformaitionMatrix.WVP);
    output.texcoord = input.texcoord;
    //法線変換
    float3 worldNormal = mul(input.normal, (float3x3) gTransformaitionMatrix.WorldInverseTranspose);
    output.normal = normalize(worldNormal);
    //output.normal = float3(0.0f, 0.0f, 1.0f); // 真っすぐ前
    return output;
}   