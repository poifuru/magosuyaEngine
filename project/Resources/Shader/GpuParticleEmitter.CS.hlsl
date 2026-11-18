#include "GpuParticle.hlsli"

ConstantBuffer<EmitterForGPU> gEmitter : register(b0);
RWStructuredBuffer<GPUParticle> gParticles : register(u0);
RWStructuredBuffer<uint> gFreeList : register(u1);
RWStructuredBuffer<int> gFreeListTop : register(u2);

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= gEmitter.emitCount)
        return;

    // 空き番号を取り出す（top には「引く前の値」が入る）
    int top;
    InterlockedAdd(gFreeListTop[0], -1, top);
    if (top < 0)
    {
        // 空きが無かった。引いた分を戻して終わる
        InterlockedAdd(gFreeListTop[0], 1);
        return;
    }
    uint index = gFreeList[top];

    uint seed = PCGHash(id.x ^ gEmitter.randomSeed);

    float3 dir = normalize(RandomInSphere(seed) + float3(0.0, 1.5, 0.0)); // 上に偏ったランダム方向
    float speed = lerp(gEmitter.speed.x, gEmitter.speed.y, Random01(seed));

    GPUParticle p = (GPUParticle) 0;
    p.position = gEmitter.position + RandomInSphere(seed) * gEmitter.radius;
    p.velocity = dir * speed;
    p.lifeTime = lerp(gEmitter.lifeTime.x, gEmitter.lifeTime.y, Random01(seed));
    p.age = 0.0;
    p.color = float4(1, 1, 1, 1);
    p.size = lerp(gEmitter.startSize.x, gEmitter.startSize.y, Random01(seed));
    p.rotation = Random01(seed) * 6.2831853; // 0〜2π
    p.rotationSpeed = (Random01(seed) * 2.0 - 1.0) * 3.0; // -3〜3 rad/s（後でエディタの項目にする）
    p.alive = 1.0;
    gParticles[index] = p;
}