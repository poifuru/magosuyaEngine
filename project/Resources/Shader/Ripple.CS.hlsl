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
    int pixelOffset = 3;
    
    // 周囲4ピクセルの1フレーム前の高さ(R)を取得
    int w = (int) width;
    int h = (int) height;
    float left = gTextures[gSimBuffer.preTextureIndex].Load(int3(max(pos.x - pixelOffset, 0), pos.y, 0)).r;
    float right = gTextures[gSimBuffer.preTextureIndex].Load(int3(min(pos.x + pixelOffset, w - 1), pos.y, 0)).r;
    float up = gTextures[gSimBuffer.preTextureIndex].Load(int3(pos.x, min(pos.y + pixelOffset, h - 1), 0)).r;
    float down = gTextures[gSimBuffer.preTextureIndex].Load(int3(pos.x, max(pos.y - pixelOffset, 0), 0)).r;
    
    // ------------------------
    // 波動方程式で波の広がりを計算
    // ------------------------
    float neighborAvg = (left + right + up + down) * 0.25f;
    
    // 波の伝播速度パラメータ（0.5～1.8程度で波の伝わる速さが変わる）
    float waveSpeed = 0.3f;
    // 減衰率（0.95～0.99程度。小さいとすぐ消え、大きいと長く残る）
    float damp = 0.996f;
    
    // 波動方程式: newHeight = (2 * current - prev + waveSpeed * (avg - current)) * damp
    float newHeight = (2.0f * currentHeight - prevHeight + waveSpeed * (neighborAvg - currentHeight)) * damp;
    
    // -----------------------------------------------
    // 前フレームの周囲4ピクセルの「泡 (B)」を取得してぼかす！
    // -----------------------------------------------
    float fLeft = gTextures[gSimBuffer.preTextureIndex].Load(int3(max(pos.x - 1, 0), pos.y, 0)).b;
    float fRight = gTextures[gSimBuffer.preTextureIndex].Load(int3(min(pos.x + 1, w - 1), pos.y, 0)).b;
    float fUp = gTextures[gSimBuffer.preTextureIndex].Load(int3(pos.x, min(pos.y + 1, h - 1), 0)).b;
    float fDown = gTextures[gSimBuffer.preTextureIndex].Load(int3(pos.x, max(pos.y - 1, 0), 0)).b;
    
    //　周囲の泡の平均（ぼかし）を取る
    float foamAvg = (fLeft + fRight + fUp + fDown) * 0.25f;
    
    // の場の泡と周囲のぼかしを少し混ぜて、点々の隙間を繋ぎ合わせる
    float blendedFoam = lerp(prevFoam, foamAvg, 0.4f);
    float newFoam = blendedFoam * 0.996f;
    
    // ---------------------------
    // オブジェクトの移動による波の注入
    // ---------------------------
     // 船の速度に応じた補間率
    float speedFactor = saturate(gSimBuffer.speed * 1.5f); // 速度に応じた強さ
    
    if (speedFactor > 0.001f)
    {
        // 船の位置をUV座標に変換
        float2 boatUV;
        boatUV.x = (gSimBuffer.position.x - gSimBuffer.waterMin.x) / gSimBuffer.waterSize.x;
        boatUV.y = 1.0f - ((gSimBuffer.position.z - gSimBuffer.waterMin.y) / gSimBuffer.waterSize.y);
        
        // ワールド空間単位の距離で計算（アスペクト比ズレを防ぐため）
        float2 diffWorld = (uv - boatUV) * gSimBuffer.waterSize;
        
        float2 fwd = normalize(gSimBuffer.forward);
        float2 rgt = float2(-fwd.y, fwd.x); // 正しい右方向ベクトル（90度回転）
        
        // 船の「左前」と「右前」の2点に波源を置く
        float bowOffset = 0.4f; // 船首の前方距離
        float widthOffset = 0.2f; // 船の横幅の半分
        
        float2 leftPoint = (fwd * bowOffset) - (rgt * widthOffset);
        float2 rightPoint = (fwd * bowOffset) + (rgt * widthOffset);
        
        float distLeft = length(diffWorld - leftPoint);
        float distRight = length(diffWorld - rightPoint);
        float distToImpact = min(distLeft, distRight);
        
        // 衝撃半径
        float impactRadius = 1.8f;
        if (distToImpact < impactRadius)
        {
            // 0.0 〜 1.0 の正規化距離
            float normDist = distToImpact / impactRadius;
            
            // ガウシアン風の滑らかな鐘型カーブ
            float shapeCurve = exp(-normDist * normDist * 3.0f);
            
            // 速度補間を掛けて、動き出しと停止を滑らかにする
            float impact = shapeCurve * 0.4f * speedFactor;
            
            newHeight += impact;
        }
        
        // -------------------------------------------------------------
        // 泡の注入（発生半径を少し広げて前後を重ね合わせる）
        // -------------------------------------------------------------
        
        // 泡は船の後ろから発生させる
        float sternOffset = 0.5f; // 船の後方へのオフセット
        float2 sternPoint = -(fwd * sternOffset); // 船の後ろ側の位置
        float distToStern = length(diffWorld - sternPoint);
    
        float sternRadius = 1.2f;
        if (distToStern < sternRadius)
        {
            float normStern = distToStern / sternRadius;
            float sternCurve = exp(-normStern * normStern * 3.0f);
        
            float foamImpact = sternCurve * 1.2f * speedFactor;
            newFoam = max(newFoam, foamImpact);
        }
    }

    // -------------------------------------------------------------
    // 結果の書き込み
    // R: 新しい高さ
    // G: 今回の高さ（次回フレームでの過去の高さ）
    // B: 泡の濃さ
    // -------------------------------------------------------------
    gOutput[id.xy] = float4(saturate(newHeight), saturate(currentHeight), saturate(newFoam), 1.0f);
}