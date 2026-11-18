struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

// エミッターの構造体
struct EmitterForGPU
{
    float3 position; // 位置
    uint emitCount; // 出現させる数
    
    float3 direction; // 向き
    float spreadAngle; // 放出方向
    
    float3 boxSize; // エミッター自体の大きさ
    uint shape; // 0:点 1:球 2:箱 3:円錐
    
    float3 acceleration; // 加速度
    float drag; // 重力や空気抵抗
    
    float4 startColor; // 出現したときの色
    
    float4 endColor; // 消滅するときの色
    
    float2 lifeTime; // 寿命
    float2 speed; // スピードの範囲(x:下限 y:上限)
    
    float2 startSize; // 出現したときの大きさ
    float endSize; // 消滅するときの大きさ
    float radius; // 半径
    
    float deltaTime; // デルタタイム
    uint randomSeed; // ランダムシード
    uint maxCount; // 最大数
    float padding;
};

// パーティクルの構造体
struct GPUParticle
{
    float3 position;
    float age;
    
    float3 velocity;
    float lifeTime;
    
    float4 color;
    
    float size;
    float rotation;
    float rotationSpeed;
    float alive;
};

// パーティクル描画用の構造体
struct ParticleDraw
{
    float4x4 viewProjection;
    
    float3 cameraRight;
    float size;
    
    float3 cameraUp;
    float padding;
    
    float4 startColor;
    
    float4 endColor;
    
    float endSize;
    float3 padding2;
};

// CPUから送るインデックス
struct PushIndices
{
    uint particleSrv;
    uint textureIndex;
};

// PCGハッシュ：整数をよく混ぜて、見た目ランダムな整数にする
uint PCGHash(uint v)
{
    uint state = v * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

// 呼ぶたびに seed を更新して 0〜1 を返す
float Random01(inout uint seed)
{
    seed = PCGHash(seed);
    return float(seed) / 4294967295.0;
}

// 半径 1 の球の中のランダムな点
float3 RandomInSphere(inout uint seed)
{
    float3 p;
    p.x = Random01(seed) * 2.0 - 1.0;
    p.y = Random01(seed) * 2.0 - 1.0;
    p.z = Random01(seed) * 2.0 - 1.0;
    return normalize(p) * pow(Random01(seed), 1.0 / 3.0); // 中心に偏らないように
}