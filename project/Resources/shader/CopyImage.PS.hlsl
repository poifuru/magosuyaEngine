#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0, space2);
SamplerState gSampler : register(s0);

struct PixelShaderOutput {
    float4 color : SV_TARGET;
};

// HDR用トーンマッピング関数
float3 ToneMapExtendedACES(float3 color, float maxScRGB)
{
    // とりあえず上限(maxScRGB)でクランプ
    return min(color, maxScRGB);
}

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    float4 color = gTexture.Sample(gSampler, input.texcoord);
    
    float maxScRGB = 800.0f / 80.0f;
    
    // 簡易トーンマッピング (Reinhardトーンマップ)
    // 1.0を超えるハイライトを綺麗に 0.0 ~ 1.0 の範囲に収める
    color.rgb = ToneMapExtendedACES(color.rgb, maxScRGB);
    
    output.color = float4(color.rgb, 1.0);
    return output;
}