#include "Object3d.hlsli"
#include "Light.hlsli"
#include "MaterialData.hlsli"

//***構造体やレジスターの定義***//
struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

struct Camera
{
    float3 worldPosition;
    float padding;
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

// 【Slot 4 (space0)】: カメラバッファ
ConstantBuffer<Camera> gCamera : register(b2, space0);

// 【Slot 5 (space0)】: ライトデータ一括
ConstantBuffer<AllLightData> gLightData : register(b3, space0);

// サンプラー (s0, space0)
SamplerState gSampler : register(s0, space0);
//******//

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
    
    if (textureColor.a <= 0.5f || myMaterial.color.a == 0.0f) { discard; }
    
    float3 totalDiffuse = float3(0, 0, 0);
    float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
    AllLightData allLights = gLightData;
    
    totalDiffuse = UpdateLights(allLights, myMaterial, input.normal, input.worldPosition, textureColor, toEye);
    
    // 環境マップは未バインドのため、0.0で安全にフォールバック
    float4 environmentColor = float4(0.0f, 0.0f, 0.0f, 0.0f);

    if (myMaterial.enableLighting == 0)
    {
        output.color = myMaterial.color * textureColor;
    }
    else
    {
        output.color.rgb = totalDiffuse + environmentColor.rgb;
        output.color.a = myMaterial.color.a * textureColor.a;
    }
    
    // 距離フェード
    //float fogT = saturate(length(gCamera.worldPosition - input.worldPosition) / 450.0f);
    //output.color.rgb = lerp(output.color.rgb, float3(0.65f, 0.82f, 0.92f), fogT * fogT * 0.85f);
    
    return output;
}