// 1: iWave / 0: 波動方程式
#define USE_IWAVE 1    
    
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
    int2 scroll;
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

// 範囲外は「静かな水面(0)」として扱う。スクロールで新しく入ってきたマスも自然に0になる
float4 LoadPrev(int2 p, int w, int h)
{
    if (p.x < 0 || p.y < 0 || p.x >= w || p.y >= h)
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    return gTextures[gSimBuffer.preTextureIndex].Load(int3(p, 0));
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) // id.xy が各ピクセルの(x.y)座標になる
{
    // テクスチャの幅と高さを取得
    uint width = (uint) gSimBuffer.textureSize.x;
    uint height = (uint) gSimBuffer.textureSize.y;
    
    // 周囲4ピクセルの1フレーム前の高さ(R)を取得
    int w = (int) width;
    int h = (int) height;
    
    // テクスチャの範囲外なら何もしないガード
    if (id.x >= width || id.y >= height)
        return;
    
    int2 pos = int2(id.xy) + gSimBuffer.scroll; // ← 前フレームでの同じ場所
    
    // ピクセル座標から0.0 ~ 1.0のUV座標を計算
    float2 uv = float2((float) id.x / width, (float) id.y / height);
    
    // ----------------------------------
    // 前フレームの高さ情報を取得
    // R: 1フレーム前の高さ (currentHeight)
    // G: 2フレーム前の高さ (prevHeight)
    // ----------------------------------
    float4 centerData = LoadPrev(pos, w, h);
    float currentHeight = centerData.r;
    float prevHeight = centerData.g;
    float prevFoam = centerData.b; // 泡の濃度
    
    // 何ピクセル隣を見るかのオフセット
    int pixelOffset = 1;
    
    // --------------
    // 波の広がりを計算
    // --------------
    float dt = gSimBuffer.deltaTime;
    float newHeight;
    
    float4 left = LoadPrev(pos + int2(-1, 0), w, h);
    float4 right = LoadPrev(pos + int2(1, 0), w, h);
    float4 up = LoadPrev(pos + int2(0, 1), w, h);
    float4 down = LoadPrev(pos + int2(0, -1), w, h);
    
    float texelSize = gSimBuffer.waterSize.x / gSimBuffer.textureSize.x;
    
#if USE_IWAVE   // iWaveによる計算
    // iWave用カーネルの畳み込み
    float vd = 0.0f;
    for (int y = -6; y <= 6; ++y)
    {
        for (int x = -6; x <= 6; ++x)
        {
            // 読む場所(端ではみ出さないようにclamp)
            int2 p = clamp(pos + int2(x, y), int2(0, 0), int2(w - 1, h - 1));
            float neighborHeight = LoadPrev(pos + int2(x, y), w, h).r;
            
            // C++で送ったカーネルから重みを取り出す
            int i = (y + 6) * 13 + (x + 6);
            float weight = gSimBuffer.iWaveKernel[i >> 2][i & 3];
            
            vd += weight * neighborHeight;
        }
    }
    
    float gravity = 9.8f / texelSize; // 実際の重力をマス単位に（約63）
    float alpha = 0.75f;     // 減衰(大きいほど早く消える)
    
    newHeight = (currentHeight * (2.0f - alpha * dt) - prevHeight - gravity * dt * dt * vd) / (1.0f + alpha * dt);
    
#else   // 波動方程式での計算
    // 斜め4点を読む
    float lu = LoadPrev(pos + int2(-1, 1), w, h).r; // 左上
    float ru = LoadPrev(pos + int2(1, 1), w, h).r; // 右上
    float ld = LoadPrev(pos + int2(-1, -1), w, h).r; // 左下
    float rd = LoadPrev(pos + int2(1, -1), w, h).r; // 右下
    
    // 9点ラプラシアン（上下左右 2/3、斜め 1/6、中心 -10/3）
    float laplacian = (left.r + right.r + up.r + down.r) * (2.0f / 3.0f)
                + (lu + ru + ld + rd) * (1.0f / 6.0f)
                - currentHeight * (10.0f / 3.0f);

    float waveSpeed = 0.7f; // 約3 m/s
    float damp = 0.993f;

    const float damping = 0.35f; // 1秒あたり
    float velocity = (currentHeight - prevHeight) * saturate(1.0f - damping * dt);
    newHeight = currentHeight + velocity + waveSpeed * laplacian * 0.25f;
#endif
    
    // -----------------------------------------------
    // 前フレームの周囲4ピクセルの「泡 (B)」を取得してぼかす
    // -----------------------------------------------
    float foamAvg = (left.b + right.b + up.b + down.b) * 0.25f;
    const float foamDecay = 0.5f; // 1秒あたりの消え方
    float newFoam = lerp(prevFoam, foamAvg, 0.2f) * exp(-foamDecay * dt);

    // 波紋の傾きが急な所に泡（航跡のV字の縁が白くなる）
    texelSize = gSimBuffer.waterSize.x / gSimBuffer.textureSize.x;
    float2 slope = float2(right.r - left.r, up.r - down.r) / (2.0f * texelSize);
    newFoam += saturate(length(slope) - 0.35f) * dt * 1.5f;
    
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
            float falloff = exp(-r2 * 1.5f);
            float2 velocity = gSimBuffer.forward;
            float speed = length(velocity);
            float speedMask = smoothstep(1.0f, 4.0f, speed);

            float push = dot(velocity, diffWorld) / impactRadius; // 前を持ち上げ、後ろを下げる
            float mexicanHat = 1.0f - 1.5f * r2; // 中心がへこみ、周りが盛り上がる（合計0）
            float displacement = -speed * 0.35f; // 船体の排水

            const float injectScale = 8.0f; // ← 航跡の強さ。ここで調整
            float force = (push * 0.5f + displacement * mexicanHat) * falloff * speedMask;
            newHeight += force * injectScale * dt * dt;
        }
            
        // -------------------------------------------------------------
        // 泡の注入
        // -------------------------------------------------------------
        float2 prevCenter = gSimBuffer.prevBoatPosition - gSimBuffer.position.xz;
        float2 seg = -prevCenter; // 前フレーム → 今
        float t = saturate(dot(diffWorld - prevCenter, seg) / max(dot(seg, seg), 1e-6f));
        float2 nearest = diffWorld - (prevCenter + seg * t);

        const float hullRadius = 1.6f;
        float hr2 = dot(nearest, nearest) / (hullRadius * hullRadius);
        if (hr2 < 6.0f)
        {
            float hullSpeed = length(gSimBuffer.forward);
            newFoam += saturate(hullSpeed * 0.08f) * exp(-hr2 * 1.5f) * dt * 4.0f;
        }
    }
    
    // 領域の端で波を吸収（跳ね返り防止）
    float edgeDist = (float) min(min(id.x, id.y), min(width - 1 - id.x, height - 1 - id.y));
    float sponge = smoothstep(0.0f, 1.0f, saturate(edgeDist / 16.0f));
    newHeight *= lerp(0.9f, 1.0f, sponge);
    currentHeight *= lerp(0.9f, 1.0f, sponge);
    newFoam *= lerp(0.95f, 1.0f, sponge);

    // -------------------------------------------------------------
    // 結果の書き込み
    // R: 新しい高さ
    // G: 今回の高さ（次回フレームでの過去の高さ）
    // B: 泡の濃さ
    // -------------------------------------------------------------
    gOutput[id.xy] = float4(clamp(newHeight, -3, 3), clamp(currentHeight, -3, 3), saturate(newFoam), 1.0f);
}