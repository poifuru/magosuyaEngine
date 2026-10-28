#pragma once

// SDF テキスト用の頂点データ受け渡し

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal   : NORMAL0;   // テキストでは法線は使わないので、x を「影かどうか」の印に使う
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    nointerpolation float isShadow : TEXCOORD1; // 1 なら影の四角形
};

// FontTextComponent から渡される、縁取り・影の設定 (b4)
struct TextEffect
{
    float4 outlineColor;
    float4 shadowColor;
    float outlineWidth;     // 縁取りの太さ（SDF の値の単位。0 で縁取り無し）
    float shadowSoftness;   // 影のぼかし（SDF の値の単位）
    float2 padding;
};
