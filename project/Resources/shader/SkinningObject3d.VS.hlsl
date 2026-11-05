#include "Object3d.hlsli"
#include "Matrix.hlsli"

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float4 weight : WEIGHT0;
    int4 index : INDEX0;
};

ConstantBuffer<TransformationMatrix> gTransformaitionMatrix : register(b0);

struct Well
{
    float4x4 skeletonSpaceMatrix;
    float4x4 skeletonSpaceInverseTransposeMatrix;
};
StructuredBuffer<Well> gMatrixPalette : register(t5);

struct Skinned
{
    float4 position;
    float3 normal;
};

// Skinningを行う関数
Skinned Skinning(VertexShaderInput input)
{
    Skinned skinned;
    
    // 最初に0で完全に初期化
    skinned.position = (float4) 0;
    skinned.normal = (float3) 0;
    
    //*** Skinningの処理***//
    // 位置の変換
    skinned.position += mul(input.position, gMatrixPalette[input.index.x].skeletonSpaceMatrix) * input.weight.x;
    skinned.position += mul(input.position, gMatrixPalette[input.index.y].skeletonSpaceMatrix) * input.weight.y;
    skinned.position += mul(input.position, gMatrixPalette[input.index.z].skeletonSpaceMatrix) * input.weight.z;
    skinned.position += mul(input.position, gMatrixPalette[input.index.w].skeletonSpaceMatrix) * input.weight.w;
    skinned.position.w = 1.0f; // 確実に1を入れる
    
    // 法線の変換
    skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[input.index.x].skeletonSpaceInverseTransposeMatrix) * input.weight.x;
    skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[input.index.y].skeletonSpaceInverseTransposeMatrix) * input.weight.y;
    skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[input.index.z].skeletonSpaceInverseTransposeMatrix) * input.weight.z;
    skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[input.index.w].skeletonSpaceInverseTransposeMatrix) * input.weight.w;
    skinned.normal = normalize(skinned.normal); // 正規化して戻してあげる
    
    //// 位置の変換
    //skinned.position += mul(gMatrixPalette[input.index.x].skeletonSpaceMatrix, input.position) * input.weight.x;
    //skinned.position += mul(gMatrixPalette[input.index.y].skeletonSpaceMatrix, input.position) * input.weight.y;
    //skinned.position += mul(gMatrixPalette[input.index.z].skeletonSpaceMatrix, input.position) * input.weight.z;
    //skinned.position += mul(gMatrixPalette[input.index.w].skeletonSpaceMatrix, input.position) * input.weight.w;
    //skinned.position.w = 1.0f; // 確実に1を入れる
    
    //// 法線の変換
    //skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[input.index.x].skeletonSpaceInverseTransposeMatrix) * input.weight.x;
    //skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[input.index.y].skeletonSpaceInverseTransposeMatrix) * input.weight.y;
    //skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[input.index.z].skeletonSpaceInverseTransposeMatrix) * input.weight.z;
    //skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[input.index.w].skeletonSpaceInverseTransposeMatrix) * input.weight.w;
    //skinned.normal = normalize(skinned.normal); // 正規化して戻してあげる
    
    return skinned;
}

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    // まずスキニングを行って、スキニング後の情報を手に入れる
    Skinned skinned = Skinning(input);
    
    output.position = mul(skinned.position, gTransformaitionMatrix.WVP);
    output.texcoord = input.texcoord;
    //法線変換
    float3 worldNormal = mul(skinned.normal, (float3x3) gTransformaitionMatrix.WorldInverseTranspose);
    output.normal = normalize(worldNormal);
    
    //カメラのワールド座標を渡す
    output.worldPosition = mul(skinned.position, gTransformaitionMatrix.World).xyz;
    return output;
}