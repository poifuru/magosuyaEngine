#pragma once

static const int kMaxWaves = 6;

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 worldPosition : POSITION0;
    float2 rippleUV : TEXCOORD1;
    float waveHeight : TEXCOORD2;
    float crestFoam : TEXCOORD3;
};

// 0〜1 の内側で 1、端の 5% でなめらかに 0 へ
float EdgeMask(float2 uv)
{
    float2 d = min(uv, 1.0f - uv);
    return smoothstep(0.0f, 0.05f, min(d.x, d.y));
}