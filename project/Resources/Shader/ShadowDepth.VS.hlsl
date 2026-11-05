#include "Matrix.hlsli"

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

struct Camera
{
    float3 worldPosition;
    float padding;
    float4x4 lightViewProj;
    uint shadowMapIndex;
    float shadowBias;
    float2 padding2;
};
ConstantBuffer<Camera> gCamera : register(b2, space0);

struct VSInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

float4 main(VSInput input) : SV_POSITION
{
    float4 worldPos = mul(input.position, gTransformationMatrix.World);
    return mul(worldPos, gCamera.lightViewProj);
}