#include "Particle.hlsli"

struct TransformaitionMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
};
StructuredBuffer<TransformaitionMatrix> gTransformaitionMatrices : register(t0);

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input, uint instanceID : SV_InstanceID)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformaitionMatrices[instanceID].WVP);
    output.texcoord = input.texcoord;
    //法線変換
    float3 worldNormal = mul(input.normal, (float3x3) gTransformaitionMatrices[instanceID].WorldInverseTranspose);
    output.normal = normalize(worldNormal);
    return output;
}