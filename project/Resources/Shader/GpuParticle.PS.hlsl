#include "GpuParticle.hlsli"

ConstantBuffer<PushIndices> gIndices : register(b1);
Texture2D g_Textures[] : register(t0, space2);
SamplerState gSampler : register(s0);

float4 main(VertexShaderOutput input) : SV_TARGET0
{
    float4 tex = g_Textures[gIndices.textureIndex].Sample(gSampler, input.texcoord);
    return tex * input.color;
}