#include "line.hlsli"

struct VertexShaderInput
{
    float3 position : POSITION0;
    float4 color : COLOR0;
};

struct LineForGPU
{
    float4x4 World;
    float4x4 WVP;
};
StructuredBuffer<LineForGPU> gLine : register(t0);

VertexShaderOutput main(VertexShaderInput input, uint instanceID : SV_InstanceID)
{
    VertexShaderOutput output;
    
    //LineForGPUを使ってoutputを計算
    output.position = mul(float4(input.position, 1.0f), gLine[instanceID].WVP);
    output.color = input.color;
    
	return output;
}