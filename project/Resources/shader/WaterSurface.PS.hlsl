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
    float2 waterMin; // 水面位置
    float2 waterSize; // 水面サイズ
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
    float normalStrength = 0.0f; // ここを 0.1 〜 0.4 で調整！小さいほど滑らかになる
    normal1.xy *= normalStrength;
    normal2.xy *= normalStrength;
    normal1.z = sqrt(saturate(1.0f - dot(normal1.xy, normal1.xy)));
    normal2.z = sqrt(saturate(1.0f - dot(normal2.xy, normal2.xy)));
    
    // 2つの法線を合成して normalize (z成分を掛け合わせて、上向きをキープしながらブレンド)
    float3 blendedNormal = normalize(float3(normal1.xy + normal2.xy, normal1.z * normal2.z));
    
    float time = material.time;
    float3 baseNormal = normalize(input.normal); // 頂点から来た元の法線

    // ワールド座標で貼る（タイルの継ぎ目が出ない）。大小2枚をずらして流す
    float2 uvA = input.worldPosition.xz * 0.08f + float2(0.60f, 0.80f) * time * 0.03f;
    float2 uvB = input.worldPosition.xz * 0.15f + float2(-0.70f, 0.45f) * time * 0.04f;
    float3 nA = g_Textures[texIndex].Sample(gSampler, uvA).xyz * 2.0f - 1.0f;
    float3 nB = g_Textures[texIndex].Sample(gSampler, uvB).xyz * 2.0f - 1.0f;

    // 遠くはモアレになるので弱める
    float cameraDistance = length(gCamera.worldPosition - input.worldPosition);
    float detailFade = saturate(1.0f - cameraDistance / 250.0f);

    // 法線マップの傾き（xy）を、水平な面の傾き（xz）として足す
    float detailStrength = 0.15f;
    float2 slope = (nA.xy + nB.xy) * detailStrength * detailFade;
    float3 worldNormal = normalize(baseNormal + float3(slope.x, 0.0f, slope.y));
    
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
    float R0 = 0.02f; // 水の正面反射率(2%)
    float fresnel = R0 + (1.0f - R0) * pow(1.0f - dotNV, 5.0f);
    
    // 手前と奥の色をブレンド
    // 手前の透き通った色
    float3 shallowColor = float3(0.0f, 0.65f, 0.55f);
    
    // ---------- 太陽 ----------
    float3 L = normalize(float3(-0.35f, 0.6f, 0.45f)); // ライトが無いとき用
    float3 sunColor = float3(1.0f, 1.0f, 1.0f) * 3.0f;
    if (gLightData.count.dirLight > 0)
    {
        L = normalize(-gLightData.dirLights[0].direction);
        sunColor = gLightData.dirLights[0].color.rgb * gLightData.dirLights[0].intensity;
    }

    float3 finalN = worldNormal;

    if (gWaterSurface.rippleTextureIndex > 0)
    {
        uint rippleIdx = gWaterSurface.rippleTextureIndex;

        // 1テクセル分のUVと、1テクセルが何mか
        float texW, texH;
        g_Textures[rippleIdx].GetDimensions(texW, texH);
        float2 du = float2(1.0f / texW, 0.0f);
        float2 dv = float2(0.0f, 1.0f / texH);
        float texelX = gWaterSurface.waterSize.x / texW;
        float texelZ = gWaterSurface.waterSize.y / texH;

        // 上下左右の高さ
        float hL = g_Textures[rippleIdx].Sample(gSampler, input.rippleUV - du).r;
        float hR = g_Textures[rippleIdx].Sample(gSampler, input.rippleUV + du).r;
        float hD = g_Textures[rippleIdx].Sample(gSampler, input.rippleUV - dv).r;
        float hU = g_Textures[rippleIdx].Sample(gSampler, input.rippleUV + dv).r;

        // 傾き（高さの変化量 / 距離）
        float dhdx = (hR - hL) / (2.0f * texelX);
        float dhdz = -(hU - hD) / (2.0f * texelZ); // VSでUVのyを反転しているので符号も反転

        // 見た目の強さ。0.3 で VS の高さと物理的に一致、上げるほど航跡が目立つ
        float rippleNormalStrength = 3.0f;

        // 高さ場の法線は (-dh/dx, 1, -dh/dz)。傾きを足し込む
        finalN = normalize(worldNormal + float3(-dhdx, 0.0f, -dhdz) * rippleNormalStrength);
    }
    
    float3 V = toEye;

    // ---------- 反射：空の色 ----------
    const float3 zenithColor = float3(0.20f, 0.45f, 0.85f); // 真上の空
    const float3 horizonColor = float3(0.65f, 0.82f, 0.92f); // 水平線の空

    float3 R = reflect(-V, finalN); // 視線が水面で跳ね返る向き
    R.y = abs(R.y); // 下向きに跳ねたら、空を見るように折り返す
    float3 skyColor = lerp(horizonColor, zenithColor, pow(saturate(R.y), 0.45f));

    // ---------- 屈折：水の中の色 ----------
    const float3 deepColor = float3(0.01f, 0.10f, 0.18f); // 深い青
    const float3 shallowTint = float3(0.05f, 0.45f, 0.45f); // 見下ろしたときの青緑
    float3 bodyColor = lerp(deepColor, shallowTint, pow(saturate(V.y), 1.5f) * 0.75f);
    
    // 太陽の方へ傾いた面は明るく、反対へ傾いた面は暗く（平らな面では変化なし）
    float tilt = dot(finalN.xz, normalize(L.xz + 0.0001f));
    bodyColor *= saturate(1.0f + tilt * 2.0f) * 0.5f + 0.5f;

    // ---------- フレネルで混ぜる ----------
    float waterF = 0.02f + 0.98f * pow(1.0f - saturate(dot(finalN, V)), 5.0f);
    float3 waterColor = lerp(bodyColor, skyColor, waterF);

    // ---------- 太陽のハイライト ----------
    float3 H = normalize(V + L);
    float spec = pow(saturate(dot(finalN, H)), 512.0f); // 512 が大きいほど小さく鋭い光
    waterColor += sunColor * spec * 0.15f;
    
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
    if (gWaterSurface.rippleTextureIndex > 0)
    {
        //if (gWaterSurface.rippleTextureIndex > 0 &&
        //input.rippleUV.x >= 0.0f && input.rippleUV.x <= 1.0f &&
        //input.rippleUV.y >= 0.0f && input.rippleUV.y <= 1.0f)
        //{
        float4 rippleData = g_Textures[gWaterSurface.rippleTextureIndex].Sample(gSampler, input.rippleUV);
        float foamAmount = rippleData.b; // CSの泡の濃さ
        
        // 大小二つのノイズを混ぜる(大きい模様 + 細かい模様)
        float n1 = foamNoise(input.worldPosition.xz * 2.0f); // 大きなまだら
        float n2 = foamNoise(input.worldPosition.xz * 6.0f); // 細かいちぎれ
        float noise = (n1 * 0.6f + n2 * 0.4f) * 0.5f + 0.5f; // 0〜1 に
        
        // 0.15 以下の薄い泡は 0、0.5 以上ではっきり白にする
        //float foamMask = smoothstep(0.05f, 0.3f, foamAmount);
        
        // 感度を持ち上げてノイズをブレンドする
        float foamFactor = saturate(foamAmount * 3.0f);
        
        if (foamFactor > 0.01f)
        {
            // 泡の濃さとノイズを合成して、粒状の泡模様を作る
            float foamPattern = saturate((foamAmount - (1.0f - noise) * 0.8f) * 3.0f);
            
            // くっきり真っ白な泡カラー（少し青みのある白）
            float3 foamColor = float3(0.96f, 0.98f, 1.0f);
            
            // 水面の色に泡を合成
            waterColor = lerp(waterColor, foamColor, foamPattern);
            
            // 泡の部分はスケスケにならないようアルファ値も1.0へ引き上げる
            finalAlpha = lerp(finalAlpha, 1.0f, foamPattern);
        }
        //}
    }
    
    // 最終的なカラーを出力
    output.color.rgb = waterColor;
    output.color.a = material.color.a * finalAlpha;
    
    return output;
}