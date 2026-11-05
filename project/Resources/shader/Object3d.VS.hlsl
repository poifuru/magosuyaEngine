#include "Object3d.hlsli"
#include "Matrix.hlsli"

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    //法線変換
    float3 worldNormal = mul(input.normal, (float3x3) gTransformationMatrix.WorldInverseTranspose);
    output.normal = normalize(worldNormal);
    
    //カメラのワールド座標を渡す
    output.worldPosition = mul(input.position, gTransformationMatrix.World).xyz;
    return output;
}   