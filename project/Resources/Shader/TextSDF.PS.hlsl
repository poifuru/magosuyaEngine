#include "TextSDF.hlsli"
#include "MaterialData.hlsli"

// SDF (Signed Distance Field) テキスト用ピクセルシェーダー
// アトラスの R には「文字の輪郭からの距離」が入っている
//   0.5 ちょうど = 輪郭、0.5 より大きい = 文字の内側、小さい = 外側
// しきい値をずらすだけで、太さ・縁取り・影のぼかしを変えられる

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

// 【Slot 6 (space0)】: 縁取り・影の設定
ConstantBuffer<TextEffect> gTextEffect : register(b4, space0);

// サンプラー (s0, space0)
SamplerState gSampler : register(s0, space0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    MaterialData myMaterial = g_BindlessBuffers[g_Indices.materialIndex][0];
    float dist = g_Textures[g_Indices.textureIndex].Sample(gSampler, input.texcoord).r;

    // 画面上で1px進むと距離がどれだけ変わるか。これを境界のぼかし幅にすると、
    // 拡大しても縮小しても輪郭が常に1px程度で滑らかになる
    float aa = max(fwidth(dist) * 0.75f, 1e-4f);

    // 縁取りを含めた外側の輪郭
    float outerEdge = 0.5f - gTextEffect.outlineWidth;

    float4 result;
    if (input.isShadow > 0.5f)
    {
        // 影: 縁取りまで含めた形を、ぼかして影の色で塗る
        float shadow = smoothstep(outerEdge - aa - gTextEffect.shadowSoftness, outerEdge + aa, dist);
        result = float4(gTextEffect.shadowColor.rgb, shadow * gTextEffect.shadowColor.a);
    }
    else
    {
        // 本体
        float fillAlpha = smoothstep(0.5f - aa, 0.5f + aa, dist);
        result = float4(myMaterial.color.rgb, fillAlpha);

        if (gTextEffect.outlineWidth > 0.0f)
        {
            // 縁取りを下に敷いて、その上に本体を重ねる
            float outlineAlpha = smoothstep(outerEdge - aa, outerEdge + aa, dist) * gTextEffect.outlineColor.a;
            float alpha = fillAlpha + outlineAlpha * (1.0f - fillAlpha);
            float3 rgb = (myMaterial.color.rgb * fillAlpha + gTextEffect.outlineColor.rgb * outlineAlpha * (1.0f - fillAlpha)) / max(alpha, 1e-5f);
            result = float4(rgb, alpha);
        }
    }

    // マテリアルの α はテキスト全体の不透明度（フェード用）として掛ける
    result.a *= myMaterial.color.a;
    if (result.a <= 0.0f)
    {
        discard;
    }

    output.color = result;
    return output;
}
