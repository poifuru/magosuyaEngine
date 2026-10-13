#pragma once

static const int kMaxWaves = 6;

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 worldPosition : POSITION0;
    float2 rippleUV : TEXCOORD1;
};