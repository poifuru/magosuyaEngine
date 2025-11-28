#include "Object3d.hlsli"

struct TransformaitionMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
};
ConstantBuffer<TransformaitionMatrix> gTransformaitionMatrix : register(b0);

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformaitionMatrix.WVP);
    output.texcoord = input.texcoord;
    //法線変換
    float3 worldNormal = mul(input.normal, (float3x3) gTransformaitionMatrix.WorldInverseTranspose);
    output.normal = normalize(worldNormal);
    return output;
}   