#include "Line.hlsli"

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

struct LineVertexData
{
    float3 position;
    float4 color;
};
StructuredBuffer<LineVertexData> gLineVertices : register(t1);

VertexShaderOutput main(VertexShaderInput input, uint instanceID : SV_InstanceID, uint vertexID : SV_VertexID)
{  
    // 描画される頂点バッファのオフセットを計算
    // 1インスタンスあたり2頂点
    uint lineIndex = instanceID;
    uint vertexOffset = lineIndex * 2 + vertexID;
    
    // 正しい位置にある頂点データをStructuredBufferから取得
    LineVertexData currentVertex = gLineVertices[vertexOffset];
    
    VertexShaderOutput output;
    
    //LineForGPUを使ってoutputを計算
    output.position = mul(float4(currentVertex.position, 1.0f), gLine[instanceID].WVP);
    output.color = currentVertex.color;
    
	return output;
}