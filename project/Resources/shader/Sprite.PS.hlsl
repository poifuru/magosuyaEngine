#include "Sprite.hlsli"
#include "MaterialData.hlsli"

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

struct PushIndices
{
    uint materialIndex;
    uint textureIndex;
};

// 【Slot 1 (space0)】: インデックス定数
ConstantBuffer<PushIndices> g_Indices : register(b1, space0);

// 【Slot 2 (space1)】: 巨大SRVヒープ (マテリアルデータ)
StructuredBuffer<MaterialData> g_BindlessBuffers[] : register(t0, space1);

// 【Slot 3 (space2)】: 巨大SRVヒープ (テクスチャ)
Texture2D g_Textures[] : register(t0, space2);

// サンプラー (s0, space0)
SamplerState gSampler : register(s0, space0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // バインドレスマテリアルの取得
    uint matIdx = g_Indices.materialIndex;
    MaterialData myMaterial = g_BindlessBuffers[matIdx][0];
    
    // バインドレステクスチャのサンプリング
    uint texIdx = g_Indices.textureIndex;
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), myMaterial.uvTransform);
    float4 textureColor = g_Textures[texIdx].Sample(gSampler, transformedUV.xy);
    
    if (textureColor.a <= 0.0f || myMaterial.color.a <= 0.0f)
    {
        discard;
    }
    
    output.color = textureColor * myMaterial.color;
    return output;
}