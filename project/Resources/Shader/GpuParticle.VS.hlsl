#include "GpuParticle.hlsli"

ConstantBuffer<ParticleDraw> gDraw : register(b0);
ConstantBuffer<PushIndices> gIndices : register(b1);
StructuredBuffer<GPUParticle> gParticleBuffers[] : register(t0, space1);

static const float2 kCorner[6] =
{
    float2(-0.5, 0.5), float2(0.5, 0.5), float2(-0.5, -0.5),
    float2(-0.5, -0.5), float2(0.5, 0.5), float2(0.5, -0.5)
};

VertexShaderOutput main(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
{
    VertexShaderOutput o = (VertexShaderOutput) 0;
    GPUParticle p = gParticleBuffers[gIndices.particleSrv][instanceId];

    if (p.alive == 0.0)
    {
        o.position = float4(0, 0, -1, 1); // クリップ空間の外→描かれない
        return o;
    }

    float t = saturate(p.age / p.lifeTime);
    float size = p.size * lerp(1.0, gDraw.endSize, t);
    float s, co;
    sincos(p.rotation, s, co);
    float2 corner = kCorner[vertexId];
    float2 rotated = float2(corner.x * co - corner.y * s, corner.x * s + corner.y * co);
    float2 c = rotated * size;
    float3 world = p.position + gDraw.cameraRight * c.x + gDraw.cameraUp * c.y;
    o.position = mul(float4(world, 1.0), gDraw.viewProjection);
    o.texcoord = kCorner[vertexId] * float2(1, -1) + 0.5;
    o.color = lerp(gDraw.startColor, gDraw.endColor, t) * p.color;
    return o;
}