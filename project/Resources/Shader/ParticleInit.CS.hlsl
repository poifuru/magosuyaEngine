#include "Particle.hlsli"

RWStructuredBuffer<GPUParticle> gParticles : register(u0);
RWStructuredBuffer<uint> gFreeList : register(u1);
RWStructuredBuffer<int> gFreeListTop : register(u2);
ConstantBuffer<EmitterForGPU> gEmitter : register(b0);

[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= gEmitter.maxCount) return;
    
    gParticles[id.x] = (GPUParticle) 0; // 全部0に
    gFreeList[id.x] = id.x; // 全部空いている
    
    if (id.x == 0)
    {
        gFreeListTop[0] = (int) gEmitter.maxCount - 1; // 一番上の位置
    }
}