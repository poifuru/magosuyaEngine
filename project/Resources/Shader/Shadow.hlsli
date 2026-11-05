#pragma once

// 1 にすると、影を受けるシェーダーが影の判定結果だけを白黒で出す（白 = 日なた, 黒 = 影）
// シェーダーは起動時にコンパイルされるので、書き換えたらゲームを再起動するだけでよい
#define SHADOW_DEBUG 0

// シャドウマップによる影の判定。1 = 光が当たる, 0 = 影
// shadowMap      : シャドウマップ（バインドレスの g_Textures[shadowMapIndex]）
// shadowSampler  : 比較サンプラー (s1)
// lightViewProj  : シャドウマップを描いたときのライトVP行列
// baseBias       : バイアスの基準値（ImGuiで調整する値）
// lightDir       : 平行光源の向き（光が進む向き）
float CalcShadow(
Texture2D shadowMap,
SamplerComparisonState shadowSampler,
float4x4 lightViewProj,
float baseBias,
float3 worldPos,
float3 normal,
float3 lightDir
)
{
    // ライト空間へ（正射影なので w は 1 だが一応割る）
    float4 lightPos = mul(float4(worldPos, 1.0f), lightViewProj);
    float3 ndc = lightPos.xyz / lightPos.w;

    // NDC(-1～1) → UV(0～1)。テクスチャは下向きが +V なので Y は反転する
    float2 uv = float2(ndc.x * 0.5f + 0.5f, -ndc.y * 0.5f + 0.5f);

    // シャドウマップの範囲外は影なし
    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f || ndc.z > 1.0f)
    {
        return 1.0f;
    }

    // 光に対して斜めの面ほどバイアスを大きくする（シャドウアクネ対策）
    float NdotL = saturate(dot(normal, -lightDir));
    float bias = max(baseBias * (1.0f - NdotL), baseBias * 0.1f);
    float depth = ndc.z - bias;

    // 3x3 PCF（周囲9か所の比較結果を平均して影の縁をなめらかにする）
    float width, height;
    shadowMap.GetDimensions(width, height);
    float2 texel = 1.0f / float2(width, height);

    float sum = 0.0f;
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            sum += shadowMap.SampleCmpLevelZero(shadowSampler, uv + float2(x, y) * texel, depth);
        }
    }
    return sum / 9.0f;
}
