#include "Line.hlsli"

// ViewProjection（共通ルートシグネチャの Slot 0 = b0）
cbuffer LineCamera : register(b0)
{
    float4x4 gViewProj;
};

struct VertexShaderInput
{
    float3 position : POSITION0; // ワールド座標
    float4 color : COLOR0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(float4(input.position, 1.0f), gViewProj);
    output.color = input.color;
    return output;
}
