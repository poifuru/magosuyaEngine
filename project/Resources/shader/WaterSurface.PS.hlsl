#include "Object3d.hlsli"
#include "Light.hlsli"
#include "MaterialData.hlsli"

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

struct WaterSurfaceInfo
{
    float amplitude;
    float frequency;
    float steepness;
    float padding;
    float2 direction;
    float2 padding1;
};

struct WaterSurface
{
    WaterSurfaceInfo waves[4];
    float time;
    int numActiveWaves;
    float nearFadeDistance;
    float farFadeDistance;
    uint rippleTextureIndex; // 波紋のテクスチャ
    float3 padding;
    float2 waterMin;         // 水面位置
    float2 waterSize;        // 水面サイズ
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

// 【Slot 6 (space0)】: 波のデータ
ConstantBuffer<WaterSurface> gWaterSurface : register(b4, space0);

// サンプラー (s0, space0)
SamplerState gSampler : register(s0, space0);

// 泡の模様を作るためのノイズ関数
float foamNoise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    float2 u = f * f * (3.0f - 2.0f * f);
    return lerp(lerp(sin(dot(i + float2(0, 0), float2(12.9898, 78.233))),
                     sin(dot(i + float2(1, 0), float2(12.9898, 78.233))), u.x),
                lerp(sin(dot(i + float2(0, 1), float2(12.9898, 78.233))),
                     sin(dot(i + float2(1, 1), float2(12.9898, 78.233))), u.x), u.y);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // マテリアルデータの取得
    uint matIndex = g_Indices.materialIndex;
    MaterialData material = g_BindlessBuffers[matIndex][0];
    
    // 水面タイリングの細かさ(グリッドに対してテクスチャを何回繰り返すか)
    float2 uvScale = float2(4.0f, 4.0f);
    
    // 1枚目と2枚目のスクロール方向・速さ(逆方向や斜めに流すと複雑に見える)
    float2 speed1 = float2(0.02f, 0.01f);
    float2 speed2 = float2(-0.015f, 0.025f);
    
    float2 uv1 = input.texcoord * uvScale + speed1 * material.time;
    float2 uv2 = input.texcoord * uvScale + speed2 * material.time;
    
    // テクスチャを取得
    uint texIndex = g_Indices.textureIndex; // 法線マップのインデックス
    
    // 2回サンプリングする
    float3 normalMap1 = g_Textures[texIndex].Sample(gSampler, uv1).rgb;
    float3 normalMap2 = g_Textures[texIndex].Sample(gSampler, uv2).rgb;
    
    // [0, 1] 範囲から [-1, 1] にデコード
    float3 normal1 = normalMap1 * 2.0f - 1.0f;
    float3 normal2 = normalMap2 * 2.0f - 1.0f;
    
    // 法線の強さ（傾き）をマイルドに補正する（XとYを小さくし、上向きZをキープ）
    float normalStrength = 0.2f; // ここを 0.1 〜 0.4 で調整！小さいほど滑らかになる
    normal1.xy *= normalStrength;
    normal2.xy *= normalStrength;
    normal1.z = sqrt(saturate(1.0f - dot(normal1.xy, normal1.xy)));
    normal2.z = sqrt(saturate(1.0f - dot(normal2.xy, normal2.xy)));
    
    // 2つの法線を合成して normalize (z成分を掛け合わせて、上向きをキープしながらブレンド)
    float3 blendedNormal = normalize(float3(normal1.xy + normal2.xy, normal1.z * normal2.z));
    
    // 簡易TBN（スクリーン座標の微分を利用して、法線マップをワールド座標系に合わせる）
    float3 N = normalize(input.normal); // 頂点シェーダーからきたワールド法線
    float3 dp1 = ddx(input.worldPosition);
    float3 dp2 = ddy(input.worldPosition);
    float2 duv1 = ddx(input.texcoord);
    float2 duv2 = ddy(input.texcoord);

    float3 dp2perp = cross(dp2, N);
    float3 dp1perp = cross(N, dp1);
    float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    float3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    float invmax = rsqrt(max(dot(T, T), dot(B, B)));
    float3x3 TBN = float3x3(T * invmax, B * invmax, N);

    // 最終的なワールド空間の法線を計算
    float3 worldNormal = normalize(mul(blendedNormal, TBN));
    
    // 最終的に出力する色
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);
    
    // 全てのライト情報を取得
    AllLightData allLights = gLightData;
    
    // テクスチャの色を取得
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), material.uvTransform);
    float4 textureColor = g_Textures[texIndex].Sample(gSampler, transformedUV.xy);
    
    // 視点のベクトル
    float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
    
    // PBRで照らされてハイライトが走るベースのカラー(深い海の色)
    float4 deepWaterColor = float4(0.005f, 0.08f, 0.2f, 1.0f);
    
    // フレネル反射
    float dotNV = saturate(dot(worldNormal, toEye));
    float R0 = 0.02f;   // 水の正面反射率(2%)
    float fresnel = R0 + (1.0f - R0) * pow(1.0f - dotNV, 5.0f);
    
    // 手前と奥の色をブレンド
    // 手前の透き通った色
    float3 shallowColor = float3(0.0f, 0.65f, 0.55f);
    
    // ベースとなる水の色をブレンドで決める
    float3 baseWaterColor = lerp(shallowColor, deepWaterColor.rgb, fresnel);
    
    // 波紋テクスチャが有効な場合、隣接テクセルの高さ差分から法線を求める
    if (gWaterSurface.rippleTextureIndex > 0)
    {
        float2 texelSize = 1.0f / 512.0f; // テクスチャ解像度に応じたオフセット
    
    // 前後左右の高さをサンプリング
        float hL = g_Textures[gWaterSurface.rippleTextureIndex].Sample(gSampler, input.rippleUV + float2(-texelSize.x, 0.0f)).r;
        float hR = g_Textures[gWaterSurface.rippleTextureIndex].Sample(gSampler, input.rippleUV + float2(texelSize.x, 0.0f)).r;
        float hD = g_Textures[gWaterSurface.rippleTextureIndex].Sample(gSampler, input.rippleUV + float2(0.0f, -texelSize.y)).r;
        float hU = g_Textures[gWaterSurface.rippleTextureIndex].Sample(gSampler, input.rippleUV + float2(0.0f, texelSize.y)).r;
    
    // UV と ワールド座標(X, Z)の向きに合わせて傾き(dh/dx, dh/dz)を計算
        float dX = hR - hL;
        float dZ = -(hU - hD); // V増加がワールド -Z なので符号反転
    
    // 傾きの強さ（ノーマル強度）を調整
        float rippleNormalStrength = 3.0f; // 好みで調整可能
        float3 rippleNormal = float3(-dX * rippleNormalStrength, 0.0f, -dZ * rippleNormalStrength);
    
    // 既存の水面法線に波紋の傾きを足して正規化
        worldNormal = normalize(worldNormal + rippleNormal);
    }
    
    // そのベース色に対してライティング（Diffuse + Specular）を適用する
    float3 totalDiffuse = UpdateLights(allLights, material, input, float4(baseWaterColor, 1.0f), toEye, worldNormal);
    
    // 環境マップは未バインドなので、0.0fでフォールバック
    float3 environmentColor = float3(0.0f, 0.0f, 0.0f);
    
    // ライティング結果
    float3 waterColor = totalDiffuse + environmentColor;
    
    float3 minAmbient = float3(0.0f, 0.25f, 0.2f); // 最低限の明るさ（暗いエメラルド）
    
    // 現在の計算結果（waterColor）がこれより暗い部分を、minAmbient で底上げする
    waterColor = max(waterColor, minAmbient);
    
    // カメラからの距離を計算
    float cameraDist = length(gCamera.worldPosition - input.worldPosition);
    //float cameraDist = 0.0f;
    
    // 距離に応じたフェードファクターを計算 (near以下なら0.0、far以上なら1.0)
    float distFactor = saturate((cameraDist - gWaterSurface.nearFadeDistance) /
                       max(0.001f, gWaterSurface.farFadeDistance - gWaterSurface.nearFadeDistance));
    
    // アルファ値を変化させる
    float finalAlpha = lerp(0.5f, 0.95f, fresnel);
    
    // 遠くに行くほど不透明(1.0)になるように補間
    finalAlpha = lerp(finalAlpha, 1.0f, distFactor);
    
    // 泡の描画部分
    //if (gWaterSurface.rippleTextureIndex > 0)
    //{
    //    //if (gWaterSurface.rippleTextureIndex > 0 &&
    //    //input.rippleUV.x >= 0.0f && input.rippleUV.x <= 1.0f &&
    //    //input.rippleUV.y >= 0.0f && input.rippleUV.y <= 1.0f)
    //    //{
    //        float4 rippleData = g_Textures[gWaterSurface.rippleTextureIndex].Sample(gSampler, input.rippleUV);
    //        float foamAmount = rippleData.b; // CSの泡の濃さ
        
    //    // 感度を持ち上げてノイズをブレンドする
    //        float foamFactor = saturate(foamAmount * 3.0f);
        
    //        if (foamFactor > 0.01f)
    //        {
    //        // ワールド座標から泡の模様を生成
    //            float noise = foamNoise(input.worldPosition.xz * 8.0f) * 0.5f + 0.5f;
            
    //        // 泡の濃さとノイズを合成して、粒状の泡模様を作る
    //            float foamPattern = saturate(foamFactor * (noise * 0.5f + 0.5f));
            
    //        // くっきり真っ白な泡カラー（少し青みのある白）
    //            float3 foamColor = float3(0.96f, 0.98f, 1.0f);
            
    //        // 水面の色に泡を合成
    //            waterColor = lerp(waterColor, foamColor, foamPattern);
            
    //        // 泡の部分はスケスケにならないようアルファ値も1.0へ引き上げる
    //            finalAlpha = lerp(finalAlpha, 1.0f, foamPattern);
    //        }
    //    //}
    //}
    
    // 最終的なカラーを出力
    output.color.rgb = waterColor;
    output.color.a = material.color.a * finalAlpha;
    
    return output;
}