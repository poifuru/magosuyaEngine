#include "Mesh.hlsli"

struct VertexShaderInput
{
    float3 position : POSITION0;
    float4 color : COLOR0;
};

struct CubeForGPU
{
    float4x4 World;
    float4x4 WVP;
};
StructuredBuffer<CubeForGPU> gCube : register(t0);

struct CubeVertexData
{
    float3 position;
    float4 color;
};
StructuredBuffer<CubeVertexData> gCubeVertices : register(t1);

VertexShaderOutput main(VertexShaderInput input, uint instanceID : SV_InstanceID, uint vertexID : SV_VertexID)
{
    // 描画される頂点バッファのオフセットを計算
    // 1インスタンスあたり2頂点
    uint cubeIndex = instanceID;
    uint vertexOffset = cubeIndex * 8 + vertexID;
    
    // 正しい位置にある頂点データをStructuredBufferから取得
    CubeVertexData currentVertex = gCubeVertices[vertexOffset];
    
    VertexShaderOutput output;
    
    //LineForGPUを使ってoutputを計算
    output.position = mul(float4(currentVertex.position, 1.0f), gCube[instanceID].WVP);
    output.color = currentVertex.color;
    
    return output;
}