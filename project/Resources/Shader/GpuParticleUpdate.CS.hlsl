#include "GpuParticle.hlsli"

ConstantBuffer<EmitterForGPU> gEmitter : register(b0);
RWStructuredBuffer<GPUParticle> gParticles : register(u0);
RWStructuredBuffer<uint> gFreeList : register(u1);
RWStructuredBuffer<int> gFreeListTop : register(u2);

[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= gEmitter.maxCount)
        return;

    GPUParticle p = gParticles[id.x];
    if (p.alive == 0.0)
        return; // 死んでいる粒は何もしない

    p.age += gEmitter.deltaTime;
    if (p.age >= p.lifeTime)
    {
        // 寿命が来た：死んだ印を付けて、自分の番号を空きリストに戻す
        p.alive = 0.0;
        int top;
        InterlockedAdd(gFreeListTop[0], 1, top); // top = 足す前の値
        gFreeList[top + 1] = id.x;
    }
    else
    {
        p.velocity += gEmitter.acceleration * gEmitter.deltaTime; // 重力
        p.velocity *= saturate(1.0 - gEmitter.drag * gEmitter.deltaTime); // 空気抵抗
        p.position += p.velocity * gEmitter.deltaTime;
        p.rotation += p.rotationSpeed * gEmitter.deltaTime;
    }
    gParticles[id.x] = p; // 死んだ場合も必ず書き戻す（alive=0 を保存するため）
}