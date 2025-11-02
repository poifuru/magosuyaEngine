#include "Object3d.hlsli"

struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    float32_t4x4 uvTransform;
};

struct DirectionalLight
{
    float32_t4 color;
    float32_t3 direction;
    float intensity;
};

ConstantBuffer<DirectionalLight> gDirectionalLight : register(b3);

ConstantBuffer<Material> gMaterial : register(b1);  

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);   

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    output.color = gMaterial.color * textureColor;
    
    float4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    //textureのアルファ値が一定以下ならその後の処理をしない(2値抜き)
    if (textureColor.a <= 0.5f)
    {
        discard;
    }
    if (output.color.a == 0.0f)
    {
        discard;
    }
    
    //Lighttingの計算	
    if (gMaterial.enableLighting == 1)  //Lightingする場合
    {
        float cos = saturate(dot(normalize(input.normal), -gDirectionalLight.direction));
        output.color.rgb = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
        output.color.a = gMaterial.color.a * textureColor.a;

    }
    else if (gMaterial.enableLighting == 2)
    {
        float NdotL = dot(normalize(input.normal), -normalize(gDirectionalLight.direction));
        float halfLambert = saturate(NdotL * 0.5 + 0.5); // 0〜1にマップ
        output.color = gMaterial.color * textureColor * gDirectionalLight.color * halfLambert * gDirectionalLight.intensity;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else    //Lightingしない場合
    {
        output.color = gMaterial.color * textureColor;
    }
    
    return output;
    
    //texcoordを見たいときの実験用
    //PixelShaderOutput output;

    // UVをそのまま色として出力する（確認用）
    //output.color = float4(input.texcoord, 0.0f, 1.0f);

    //return output;
}