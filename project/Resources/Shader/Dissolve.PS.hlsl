#include "Fullscreen.hlsli"

struct Dissolve
{
    float4 edgeColor;
    float threshold;
    float edgeWidth;
};

ConstantBuffer<Dissolve> gDissolve : register(b0);
Texture2D<float4> gTexture : register(t0, space2);
Texture2D<float4> gMaskTexture : register(t0, space1);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 originalColor = gTexture.Sample(gSampler, input.texcoord);
    
    // マスクテクスチャの輝度値を取得 (Rチャンネルを使用)
    float maskValue = gMaskTexture.Sample(gSampler, input.texcoord).r;
    
    // 閾値以下なら、消えた状態（ここでは黒）にする
    if (maskValue <= gDissolve.threshold)
    {
        output.color = float4(0.0f, 0.0f, 0.0f, 1.0f);
        return output;
    }
    
    // 境界線（エッジ）の判定
    if (maskValue <= gDissolve.threshold + gDissolve.edgeWidth)
    {
        // 境界線に近いほどエッジカラーを強くする
        float edgeLerp = (maskValue - gDissolve.threshold) / gDissolve.edgeWidth;
        
        output.color.rgb = lerp(gDissolve.edgeColor.rgb, originalColor.rgb, edgeLerp);
        output.color.a = originalColor.a;
        return output;
    }
    
    output.color = originalColor;
    return output;
}