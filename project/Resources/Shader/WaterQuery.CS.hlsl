#include "WaterSurface.hlsli"

static const uint kMaxQueries = 16; // 扱う頂点の最大数

struct WaterQueryParams
{
    float4 points[kMaxQueries]; // xy = ワールドの (x, z)。zw は未使用
    float2 waterMin; // 波紋シミュの範囲（BoatWakeComponent::GetSimMin）
    float2 waterSize; // 波紋シミュのサイズ（80, 80）
    uint rippleTextureIndex; // 最新の波紋テクスチャ
    uint count; // 有効な点の数
    float2 padding;
};

ConstantBuffer<WaterQueryParams> gQuery : register(b0);
RWStructuredBuffer<float4> gResult : register(u0); // x=高さ, y=dh/dx, z=dh/dz, w=範囲内かどうか(0〜1)
Texture2D gTextures[] : register(t0, space2);

float LoadHeight(int2 p, int2 size)
{
    p = clamp(p, int2(0, 0), size - 1);
    return gTextures[gQuery.rippleTextureIndex].Load(int3(p, 0)).r;
}

// 水面VSの SampleLevel（バイリニア）と同じ結果になるように手で補間する
float SampleHeight(float2 uv, int2 size)
{
    float2 pix = uv * float2(size) - 0.5f; // テクセル中心に合わせる
    int2 i0 = (int2) floor(pix);
    float2 f = pix - floor(pix);
    float h00 = LoadHeight(i0, size);
    float h10 = LoadHeight(i0 + int2(1, 0), size);
    float h01 = LoadHeight(i0 + int2(0, 1), size);
    float h11 = LoadHeight(i0 + int2(1, 1), size);
    return lerp(lerp(h00, h10, f.x), lerp(h01, h11, f.x), f.y);
}

[numthreads(16, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= gQuery.count)
        return;

    uint w, h;
    gTextures[gQuery.rippleTextureIndex].GetDimensions(w, h);
    int2 size = int2(w, h);

    float2 xz = gQuery.points[id.x].xy;

    // 水面VS・波紋CSと同じ「ワールド → UV」変換（v は反転している）
    float2 uv;
    uv.x = (xz.x - gQuery.waterMin.x) / gQuery.waterSize.x;
    uv.y = 1.0f - ((xz.y - gQuery.waterMin.y) / gQuery.waterSize.y);

    if (any(uv < 0.0f) || any(uv > 1.0f))
    {
        gResult[id.x] = float4(0, 0, 0, 0); // 範囲外は静かな水面
        return;
    }

    float texel = gQuery.waterSize.x / (float) w; // 約0.156m
    float2 du = float2(1.0f / w, 0.0f);
    float2 dv = float2(0.0f, 1.0f / h);

    float height = SampleHeight(uv, size);

    // 傾き（中心差分）。z が増えると v は減るので、z 方向は引き算の順番が逆
    float dhdx = (SampleHeight(uv + du, size) - SampleHeight(uv - du, size)) / (2.0f * texel);
    float dhdz = (SampleHeight(uv - dv, size) - SampleHeight(uv + dv, size)) / (2.0f * texel);

    float mask = EdgeMask(uv);
    gResult[id.x] = float4(height * mask, dhdx * mask, dhdz * mask, mask);
}