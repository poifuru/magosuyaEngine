// 1: iWave / 0: 波動方程式
#define USE_IWAVE 0    
    
struct SimulationBuffer
{
    float3 position; // 物体のワールド座標
    float time; // 経過時間
    
    float2 textureSize; // テクスチャの解像度
    float deltaTime; // デルタタイム
    float padding;
    
    float2 waterMin; // 水面の最小座標 (例: 中心座標 - サイズ*0.5)
    float2 waterSize; // 水面のサイズ (幅, 奥行き)
    
    uint preTextureIndex; // 前フレームのテクスチャインデックス
    float speed;
    float2 forward; // 前方ベクトル
    
    float4 iWaveKernel[43]; // iWave用のカーネル
    
    float2 prevBoatPosition; // 前フレームの船の位置 (x, z)
    float2 padding2;
};

ConstantBuffer<SimulationBuffer> gSimBuffer : register(b0);

// CSから書き込むUAVテクスチャ(Slot u0)
RWTexture2D<float4> gOutput : register(u0);

// 前フレームの波紋テクスチャを読むためのバインドレス空間
Texture2D gTextures[] : register(t0, space2);

// 簡易的なランダムノイズ関数
float hash(float2 p)
{
    return frac(sin(dot(p, float2(127.1, 311.7))) * 43758.5453);
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) // id.xy が各ピクセルの(x.y)座標になる
{
    // テクスチャの幅と高さを取得
    uint width = (uint) gSimBuffer.textureSize.x;
    uint height = (uint) gSimBuffer.textureSize.y;
    
    // テクスチャの範囲外なら何もしないガード
    if (id.x >= width || id.y >= height)
        return;
    
    int2 pos = int2(id.xy);
    
    // ピクセル座標から0.0 ~ 1.0のUV座標を計算
    float2 uv = float2((float) id.x / width, (float) id.y / height);
    
    // ----------------------------------
    // 前フレームの高さ情報を取得
    // R: 1フレーム前の高さ (currentHeight)
    // G: 2フレーム前の高さ (prevHeight)
    // ----------------------------------
    float4 centerData = gTextures[gSimBuffer.preTextureIndex].Load(int3(pos, 0));
    float currentHeight = centerData.r;
    float prevHeight = centerData.g;
    float prevFoam = centerData.b; // 泡の濃度
    
    // 何ピクセル隣を見るかのオフセット
    int pixelOffset = 1;
    
    // 周囲4ピクセルの1フレーム前の高さ(R)を取得
    int w = (int) width;
    int h = (int) height;
    
    // --------------
    // 波の広がりを計算
    // --------------
    float dt = gSimBuffer.deltaTime;
    float newHeight;
    
#if USE_IWAVE   // iWaveによる計算
    // iWave用カーネルの畳み込み
    float vd = 0.0f;
    for (int y = -6; y <= 6; ++y)
    {
        for (int x = -6; x <= 6; ++x)
        {
            // 読む場所(端ではみ出さないようにclamp)
            int2 p = clamp(pos + int2(x, y), int2(0, 0), int2(w - 1, h - 1));
            float neighborHeight = gTextures[gSimBuffer.preTextureIndex].Load(int3(p, 0)).r;
            
            // C++で送ったカーネルから重みを取り出す
            int i = (y + 6) * 13 + (x + 6);
            float weight = gSimBuffer.iWaveKernel[i >> 2][i & 3];
            
            vd += weight * neighborHeight;
        }
    }
    
    float gravity = 12.0f;  // 波の速さ(大きいほど早く伝わる)
    float alpha = 0.75f;     // 減衰(大きいほど早く消える)
    
    float newHeight = (currentHeight * (2.0f - alpha * dt) - prevHeight - gravity * dt * dt * vd) / (1.0f + alpha * dt);
    
#else   // 波動方程式での計算
    float4 left = gTextures[gSimBuffer.preTextureIndex].Load(int3(max(pos.x - 1, 0), pos.y, 0));
    float4 right = gTextures[gSimBuffer.preTextureIndex].Load(int3(min(pos.x + 1, w - 1), pos.y, 0));
    float4 up = gTextures[gSimBuffer.preTextureIndex].Load(int3(pos.x, min(pos.y + 1, h - 1), 0));
    float4 down = gTextures[gSimBuffer.preTextureIndex].Load(int3(pos.x, max(pos.y - 1, 0), 0));
    // 斜め4点も読む
    float lu = gTextures[gSimBuffer.preTextureIndex].Load(int3(pos + int2(-1, 1), 0)).r; // 左上
    float ru = gTextures[gSimBuffer.preTextureIndex].Load(int3(pos + int2(1, 1), 0)).r; // 右上
    float ld = gTextures[gSimBuffer.preTextureIndex].Load(int3(pos + int2(-1, -1), 0)).r; // 左下
    float rd = gTextures[gSimBuffer.preTextureIndex].Load(int3(pos + int2(1, -1), 0)).r; // 右下
    
    // 9点ラプラシアン（上下左右 2/3、斜め 1/6、中心 -10/3）
    float laplacian = (left.r + right.r + up.r + down.r) * (2.0f / 3.0f)
                + (lu + ru + ld + rd) * (1.0f / 6.0f)
                - currentHeight * (10.0f / 3.0f);

    float waveSpeed = 0.26f; // 約3 m/s
    float damp = 0.993f;

    float velocity = currentHeight - prevHeight;
    newHeight = (currentHeight + velocity + waveSpeed * laplacian * 0.25f) * damp;
#endif
    
    // -----------------------------------------------
    // 前フレームの周囲4ピクセルの「泡 (B)」を取得してぼかす
    // -----------------------------------------------
    float fLeft = gTextures[gSimBuffer.preTextureIndex].Load(int3(max(pos.x - 1, 0), pos.y, 0)).b;
    float fRight = gTextures[gSimBuffer.preTextureIndex].Load(int3(min(pos.x + 1, w - 1), pos.y, 0)).b;
    float fUp = gTextures[gSimBuffer.preTextureIndex].Load(int3(pos.x, min(pos.y + 1, h - 1), 0)).b;
    float fDown = gTextures[gSimBuffer.preTextureIndex].Load(int3(pos.x, max(pos.y - 1, 0), 0)).b;
    
    //　周囲の泡の平均（ぼかし）を取る
    float foamAvg = (fLeft + fRight + fUp + fDown) * 0.25f;
    
    float blendedFoam = lerp(prevFoam, foamAvg, 0.1f); // 0.4 → 0.1（にじみを弱く）
    float newFoam = blendedFoam * 0.985f; // 0.996 → 0.985（1秒で約6割消える）
    
    // ---------------------------
    // オブジェクトの移動による波の注入
    // ---------------------------
     // 船の速度に応じた補間率
    float speedFactor = saturate(length(gSimBuffer.forward) / 10.0f); // 速度に応じた強さ
    
    if (speedFactor > 0.001f)
    {
        // 船の位置をUV座標に変換
        float2 boatUV;
        boatUV.x = (gSimBuffer.position.x - gSimBuffer.waterMin.x) / gSimBuffer.waterSize.x;
        boatUV.y = 1.0f - ((gSimBuffer.position.z - gSimBuffer.waterMin.y) / gSimBuffer.waterSize.y);
        
        // ワールド空間単位の距離で計算（アスペクト比ズレを防ぐため）
        float2 diffWorld = (uv - boatUV) * gSimBuffer.waterSize;
        diffWorld.y = -diffWorld.y;
        
        float2 fwd = normalize(gSimBuffer.forward);
        float2 rgt = float2(-fwd.y, fwd.x); // 正しい右方向ベクトル（90度回転）
        
        // 船の「左前」と「右前」の2点に波源を置く
        float bowOffset = 0.4f; // 船首の前方距離
        float widthOffset = 0.2f; // 船の横幅の半分
        
        // 衝撃半径
        float impactRadius = 1.5f;
       
        float r2 = dot(diffWorld, diffWorld) / (impactRadius * impactRadius);
        if (r2 < 6.0f)
        {
            float falloff = exp(-r2 * 1.5f); // 釣り鐘カーブ
            
            float2 velocity = gSimBuffer.forward; // m/sの速度ベクトル
            float push = dot(velocity, diffWorld) / impactRadius; // 前+ / 後ろ- / 横0
            
            float speed = length(velocity);
            float speedMask = smoothstep(1.0f, 4.0f, speed);
            float accel = push * 2.0f * falloff * speedMask; // 加速度[m/s^2]
            float dt = gSimBuffer.deltaTime;
            
            newHeight += accel * dt * dt;
        }
            
        // -------------------------------------------------------------
        // 泡の注入（発生半径を少し広げて前後を重ね合わせる）
        // -------------------------------------------------------------
        // 泡は船の後ろから発生させる
        float sternOffset = 0.3f; // 船の後方へのオフセット
        float blendedFoam = lerp(prevFoam, foamAvg, 0.02f); // 0.1 → 0.02(扇形に広がる原因)
        float2 sternPoint = -(fwd * sternOffset); // 船の後ろ側の位置
        
        // 前フレームの船尾の位置（今の船の位置からの相対座標）
        float2 prevSternPoint = (gSimBuffer.prevBoatPosition - gSimBuffer.position.xz) + sternPoint;
        
        // 前フレームの船尾 → 今の船尾を結ぶ線分までの距離（フレーム間のすき間を埋める）
        float2 segment = sternPoint - prevSternPoint;
        float t = saturate(dot(diffWorld - prevSternPoint, segment) / max(dot(segment, segment), 1e-6f));
        float distToStern = length(diffWorld - (prevSternPoint + segment * t));
    
        float sternRadius = 0.7f;
        if (distToStern < sternRadius)
        {
            float normStern = distToStern / sternRadius;
            float sternCurve = exp(-normStern * normStern * 3.0f);
        
            float foamImpact = sternCurve * 2.0f * speedFactor;
            newFoam = saturate(blendedFoam * 0.993f) + foamImpact; // にじまない分、長めに残す
        }
    }

    // -------------------------------------------------------------
    // 結果の書き込み
    // R: 新しい高さ
    // G: 今回の高さ（次回フレームでの過去の高さ）
    // B: 泡の濃さ
    // -------------------------------------------------------------
    gOutput[id.xy] = float4(clamp(newHeight, -3, 3), clamp(currentHeight, -3, 3), clamp(newFoam, -3, 3), 1.0f);
}