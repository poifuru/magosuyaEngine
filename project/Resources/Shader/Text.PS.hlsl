#include "Sprite.hlsli"
#include "MaterialData.hlsli"

// フォントのテキスト用ピクセルシェーダー
// アトラスは R8 (1ch) で、R の値が「その画素を文字がどれだけ覆っているか」
// それを不透明度として使い、色はマテリアルの color で決める

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
    
    // アトラスから覆い率を読む
    uint texIdx = g_Indices.textureIndex;
    float coverage = g_Textures[texIdx].Sample(gSampler, input.texcoord).r;
    
    float alpha = coverage * myMaterial.color.a;
    if (alpha <= 0.0f)
    {
        discard;
    }
    
    output.color = float4(myMaterial.color.rgb, alpha);
    return output;
}
