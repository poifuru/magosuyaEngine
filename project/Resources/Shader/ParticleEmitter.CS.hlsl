#include "Particle.hlsli"

ConstantBuffer<EmitterForGPU> gEmitter : register(b0);

[numthreads(64, 1, 1)]
void main( uint3 id : SV_DispatchThreadID )
{
    if (id.x >= gEmitter.emitCount) return;

    int top;
    InterlockedAdd(gFreeLisTop)
}