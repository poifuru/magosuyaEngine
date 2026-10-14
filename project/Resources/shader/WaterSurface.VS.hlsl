#include "WaterSurface.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
};

struct WaterSurfaceInfo
{
    float amplitude; // 振幅(A)
    float frequency; // 周波数(w)
    float steepness; // 険しさ(Q)
    float padding;
    float2 direction; // 方向(D)
    float2 padding1;
};

struct WaterSurface
{
    WaterSurfaceInfo waves[kMaxWaves]; // 4つの波の情報（32バイト * 4 = 128バイト）
    float time;                 // 時間
    int numActiveWaves;         // 現在有効にする波の数（1〜4）
    float nearFadeDistance;
    float farFadeDistance;
    uint rippleTextureIndex;    // 波紋のテクスチャ
    float3 padding;
    float2 waterMin;            // 水面位置
    float2 waterSize;           // 水面サイズ
};

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

Texture2D gTextures[] : register(t0, space2);
SamplerState gSampler : register(s0, space0);
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);
ConstantBuffer<WaterSurface> gWaterSurface : register(b4);

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    // 元の頂点座標をコピー
    float3 position = input.position.xyz;
    
     // 複数の波の変位を累積する変数
    float3 waveOffset = float3(0.0f, 0.0f, 0.0f);
    
    // 接線と従法線の初期値(変形前はそれぞれX方向、Z方向)
    float3 tangent = float3(1.0f, 0.0f, 0.0f);
    float3 binormal = float3(0.0f, 0.0f, 1.0f);
    
    // ワールド座標で計算する（タイルの継ぎ目が出ない）
    float3 worldBase = mul(float4(input.position.xyz, 1.0f), gTransformationMatrix.World).xyz;
    float2 xz = worldBase.xz;

    float t = gWaterSurface.time;
    int count = gWaterSurface.numActiveWaves;

    for (int i = 0; i < min(count, kMaxWaves); ++i)
    {
        float A = gWaterSurface.waves[i].amplitude;
        float k = gWaterSurface.waves[i].frequency; // 波数 k = 2π/波長
        float2 D = normalize(gWaterSurface.waves[i].direction);

        float omega = sqrt(9.8f * k); // 深い水の分散関係：長い波ほど速い
        float Q = gWaterSurface.waves[i].steepness / (k * A * count + 1e-4f); // 波を重ねても裏返らない

        float theta = k * dot(D, xz) - omega * t + i * 2.4f; // i*2.4 は波ごとの位相のずらし
        float s, c;
        sincos(theta, s, c);

        waveOffset.x += Q * A * D.x * c;
        waveOffset.z += Q * A * D.y * c;
        waveOffset.y += A * s;

        float WA = k * A;
        tangent.x -= Q * D.x * D.x * WA * s;
        tangent.y += D.x * WA * c;
        tangent.z -= Q * D.x * D.y * WA * s;
        binormal.x -= Q * D.x * D.y * WA * s;
        binormal.y += D.y * WA * c;
        binormal.z -= Q * D.y * D.y * WA * s;
    }
    
    float jacobian = tangent.x * binormal.z - binormal.x * tangent.z;
    output.crestFoam = saturate((0.8f - jacobian) * 3.0f);
    
    // 頂点のワールド座標(X, Z)を計算！
    float3 worldPos = worldBase + waveOffset; // ワールドで足す
    
    // WVPはローカル座標用なので、ずれをスケールで割ってローカルに戻す
    float3 axisScale = float3(length(gTransformationMatrix.World[0].xyz),
                          length(gTransformationMatrix.World[1].xyz),
                          length(gTransformationMatrix.World[2].xyz));
    position += waveOffset / axisScale;
    
    // CSと全く同じ式でワールド座標 ➔ UV 座標に変換
    float2 rippleUV;
    rippleUV.x = (worldPos.x - gWaterSurface.waterMin.x) / gWaterSurface.waterSize.x;
    rippleUV.y = 1.0f - ((worldPos.z - gWaterSurface.waterMin.y) / gWaterSurface.waterSize.y);
    
    output.rippleUV = rippleUV;
    
    // CSで作った波紋を組み込む
     // インデックスが 0 以外（有効なテクスチャ）の時だけサンプリング
    float rippleHeight = 0.0f;
    if (gWaterSurface.rippleTextureIndex > 0)
    {
        //if (gWaterSurface.rippleTextureIndex > 0 &&
        //rippleUV.x >= 0.0f && rippleUV.x <= 1.0f &&
        //rippleUV.y >= 0.0f && rippleUV.y <= 1.0f)
        //{
            // 頂点のuv座標を使ってCSのテクスチャをサンプリングする
            float4 rippleData = gTextures[gWaterSurface.rippleTextureIndex].SampleLevel(gSampler, rippleUV, 0);
    
            // CSが書いた波の高さをY座標に加算
            float rippleMask = RippleEdgeMask(rippleUV);
            rippleHeight = rippleData.r * 1.0f * rippleMask;
            position.y += rippleHeight;
        //}
    }
    
    // w成分(1.0f)を新たに追加して4次元ベクトルを完成させる
    float4 finalPosition = float4(position, 1.0f);
    
    // 変形後の座標をWVP変換
    output.position = mul(finalPosition, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    
    // 二つのベクトルの外積から法線を計算して正規化する
    // 左手系において、binormal × tangent で上向きの法線が求まる
    float3 localNormal = normalize(cross(binormal, tangent));
    
    // 法線をワールド空間に変換してピクセルシェーダーに渡す
    float3 worldNormal = mul(localNormal, (float3x3) gTransformationMatrix.WorldInverseTranspose);
    output.normal = normalize(worldNormal);
    
    output.worldPosition = mul(finalPosition, gTransformationMatrix.World).xyz;
    
    output.waveHeight = waveOffset.y + rippleHeight;
    
    return output;
}