#pragma once
#include "MaterialData.hlsli"

static const float PI = 3.14159265359f;

struct LightCount
{
    int dirLight;
    int pointLight;
    int spotLight;
    int rectLight;
};

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

struct PointLight
{
    float4 color;
    float3 position;
    float intensity;
    float radius; //ライトが届く最大距離
    float decay; //減衰率
    float padding[2];
};

struct SpotLight
{
    float4 color;
    float3 position;
    float intensity;
    float3 direction;
    float distance;
    float decay;
    float cosAngle;
    float padding[2];
};

struct RectLight
{
    float4 color;
    float3 position; // ライトの中心座標
    float intensity;
    float3 direction; // ライトの正面方向（法線）
    float padding1;
    float2 size; // Width(幅) と Height(高さ)
    float padding2[2];
    float3 right; // ライトの右方向ベクトル
    float padding;
    float3 up; // ライトの上方向ベクトル
    float decay; // 距離による減衰率（PointLightと同様）
};

struct AllLightData
{
    float4 ambientColor; // 環境光の色 (RGB) と 強さ (A)
    LightCount count;
    DirectionalLight dirLights[50];
    PointLight pointLights[50];
    SpotLight spotLights[50];
    RectLight rectLights[50];
};

float GeometrySchlickGGX(float NdotV, float k)
{
    return NdotV / (NdotV * (1.0f - k) + k);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = saturate(dot(N, H));
    float denom = (NdotH * NdotH * (a2 - 1.0f) + 1.0f);
    return a2 / (PI * denom * denom);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;
    
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    
    float ggx2 = GeometrySchlickGGX(NdotV, k);
    float ggx1 = GeometrySchlickGGX(NdotL, k);
    
    return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

float3 UpdateLights(
AllLightData allLights,
MaterialData material,
float3 normal,
float3 worldPosition,
float4 color,
float3 toEye
)
{
    float3 totalDiffuse = float3(0.0f, 0.0f, 0.0f);

    // DirectionalLights
    for (int i = 0; i < allLights.count.dirLight; ++i)
    {
        float3 L = normalize(-allLights.dirLights[i].direction);
        float3 N = normalize(normal);
        float3 V = toEye;
        float3 H = normalize(V + L);

        float3 lightColor = allLights.dirLights[i].color.rgb;
        float intensity = allLights.dirLights[i].intensity;

        float roughness = saturate(material.roughness);
        float metallic = saturate(material.metallic);
        float3 albedo = material.color.rgb * color.rgb;
        float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

        float D = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = FresnelSchlick(saturate(dot(H, V)), F0);

        float3 specular = (D * G * F) / max(4.0f * saturate(dot(N, V)) * saturate(dot(N, L)), 0.01f);
        float3 kD = (float3(1.0f, 1.0f, 1.0f) - F) * (1.0f - metallic);
        float3 diffuse = kD * albedo / PI;

        totalDiffuse += (diffuse + specular) * lightColor * intensity * saturate(dot(N, L));
    }
    
    // PointLight
    for (int j = 0; j < allLights.count.pointLight; ++j)
    {
        float3 direction = worldPosition - allLights.pointLights[j].position;
        float distance = length(direction);
        float3 L = normalize(-direction);
        float3 N = normalize(normal);
        float3 V = toEye;
        float3 H = normalize(V + L);

        float attenuation = pow(saturate(1.0f - (distance / allLights.pointLights[j].radius)), allLights.pointLights[j].decay);
        float3 lightColor = allLights.pointLights[j].color.rgb;
        float intensity = allLights.pointLights[j].intensity * attenuation;

        float roughness = saturate(material.roughness);
        float metallic = saturate(material.metallic);
        float3 albedo = material.color.rgb * color.rgb;

        float3 F0 = float3(0.04f, 0.04f, 0.04f);
        F0 = lerp(F0, albedo, metallic);

        float D = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = FresnelSchlick(saturate(dot(H, V)), F0);

        float3 numerator = D * G * F;
        float denominator = 4.0f * saturate(dot(N, V)) * saturate(dot(N, L)) + 0.0001f;
        float3 specular = numerator / denominator;

        float3 kS = F;
        float3 kD = (float3(1.0f, 1.0f, 1.0f) - kS) * (1.0f - metallic);
        float3 diffuse = kD * albedo / PI;

        float nDotL = saturate(dot(N, L));
        totalDiffuse += (diffuse + specular) * lightColor * intensity * nDotL;
    }
    
    // SpotLight
    for (int k = 0; k < allLights.count.spotLight; ++k)
    {
        float3 direction = worldPosition - allLights.spotLights[k].position;
        float distance = length(direction);
        float3 L = normalize(-direction);
        float3 N = normalize(normal);
        float3 V = toEye;
        float3 H = normalize(V + L);

        float attenuation = pow(saturate(1.0f - (distance / allLights.spotLights[k].distance)), allLights.spotLights[k].decay);
        float cosToPos = dot(-L, normalize(allLights.spotLights[k].direction));
        float spotFactor = saturate((cosToPos - allLights.spotLights[k].cosAngle) / (1.0f - allLights.spotLights[k].cosAngle));
        float intensity = allLights.spotLights[k].intensity * attenuation * spotFactor;

        float3 lightColor = allLights.spotLights[k].color.rgb;

        float roughness = saturate(material.roughness);
        float metallic = saturate(material.metallic);
        float3 albedo = material.color.rgb * color.rgb;
        float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

        float D = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = FresnelSchlick(saturate(dot(H, V)), F0);

        float3 specular = (D * G * F) / max(4.0f * saturate(dot(N, V)) * saturate(dot(N, L)), 0.01f);
        float3 kD = (float3(1.0f, 1.0f, 1.0f) - F) * (1.0f - metallic);
        float3 diffuse = kD * albedo / PI;

        totalDiffuse += (diffuse + specular) * lightColor * intensity * saturate(dot(N, L));
    }
    
    // RectLight
    for (int l = 0; l < allLights.count.rectLight; ++l)
    {
        float3 halfW = allLights.rectLights[l].right * (allLights.rectLights[l].size.x * 0.5f);
        float3 halfH = allLights.rectLights[l].up * (allLights.rectLights[l].size.y * 0.5f);
        
        float3 p[4];
        p[0] = allLights.rectLights[l].position - halfW - halfH; // 左下
        p[1] = allLights.rectLights[l].position + halfW - halfH; // 右下
        p[2] = allLights.rectLights[l].position + halfW + halfH; // 右上
        p[3] = allLights.rectLights[l].position - halfW + halfH; // 左上

        float3 v[4];
        for (int idx = 0; idx < 4; idx++)
        {
            v[idx] = normalize(p[idx] - worldPosition);
        }

        float illuminance = 0.0f;
        illuminance += acos(saturate(dot(v[0], v[1])));
        illuminance += acos(saturate(dot(v[1], v[2])));
        illuminance += acos(saturate(dot(v[2], v[3])));
        illuminance += acos(saturate(dot(v[3], v[0])));
        
        illuminance /= (2.0f * PI);

        float3 distVec = worldPosition - allLights.rectLights[l].position;
        float distance = length(distVec);
        float attenuation = pow(saturate(1.0f - (distance / 20.0f)), allLights.rectLights[l].decay);

        float3 N = normalize(normal);
        float3 L = normalize(-allLights.rectLights[l].direction);
        float nDotL = saturate(dot(N, L));

        float3 albedo = material.color.rgb * color.rgb;
        float3 diffuse = (albedo / PI) * allLights.rectLights[l].color.rgb * allLights.rectLights[l].intensity * illuminance * nDotL * attenuation;

        totalDiffuse += diffuse;
    }
    
    // アルベドの計算
    float3 albedo = material.color.rgb * color.rgb;
    
    // 環境光の計算
    float3 ambient = allLights.ambientColor.rgb * albedo;
    
    // 直接光と環境光を合成
    return totalDiffuse + ambient;
}

float3 UpdateLights(
AllLightData allLights,
MaterialData material,
float3 worldPosition,
float4 color,
float3 toEye,
float3 worldNormal
)
{
    float3 totalDiffuse = float3(0.0f, 0.0f, 0.0f);

    // DirectionalLights
    for (int i = 0; i < allLights.count.dirLight; ++i)
    {
        float3 L = normalize(-allLights.dirLights[i].direction);
        float3 N = normalize(worldNormal);
        float3 V = toEye;
        float3 H = normalize(V + L);

        float3 lightColor = allLights.dirLights[i].color.rgb;
        float intensity = allLights.dirLights[i].intensity;

        float roughness = saturate(material.roughness);
        float metallic = saturate(material.metallic);
        float3 albedo = material.color.rgb * color.rgb;
        float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

        float D = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = FresnelSchlick(saturate(dot(H, V)), F0);

        float3 specular = (D * G * F) / max(4.0f * saturate(dot(N, V)) * saturate(dot(N, L)), 0.01f);
        float3 kD = (float3(1.0f, 1.0f, 1.0f) - F) * (1.0f - metallic);
        float3 diffuse = kD * albedo / PI;

        totalDiffuse += (diffuse + specular) * lightColor * intensity * saturate(dot(N, L));
    }
    
    // PointLight
    for (int j = 0; j < allLights.count.pointLight; ++j)
    {
        float3 direction = worldPosition - allLights.pointLights[j].position;
        float distance = length(direction);
        float3 L = normalize(-direction);
        float3 N = normalize(worldNormal);
        float3 V = toEye;
        float3 H = normalize(V + L);

        float attenuation = pow(saturate(1.0f - (distance / allLights.pointLights[j].radius)), allLights.pointLights[j].decay);
        float3 lightColor = allLights.pointLights[j].color.rgb;
        float intensity = allLights.pointLights[j].intensity * attenuation;

        float roughness = saturate(material.roughness);
        float metallic = saturate(material.metallic);
        float3 albedo = material.color.rgb * color.rgb;

        float3 F0 = float3(0.04f, 0.04f, 0.04f);
        F0 = lerp(F0, albedo, metallic);

        float D = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = FresnelSchlick(saturate(dot(H, V)), F0);

        float3 numerator = D * G * F;
        float denominator = 4.0f * saturate(dot(N, V)) * saturate(dot(N, L)) + 0.0001f;
        float3 specular = numerator / denominator;

        float3 kS = F;
        float3 kD = (float3(1.0f, 1.0f, 1.0f) - kS) * (1.0f - metallic);
        float3 diffuse = kD * albedo / PI;

        float nDotL = saturate(dot(N, L));
        totalDiffuse += (diffuse + specular) * lightColor * intensity * nDotL;
    }
    
    // SpotLight
    for (int k = 0; k < allLights.count.spotLight; ++k)
    {
        float3 direction = worldPosition - allLights.spotLights[k].position;
        float distance = length(direction);
        float3 L = normalize(-direction);
        float3 N = normalize(worldNormal);
        float3 V = toEye;
        float3 H = normalize(V + L);

        float attenuation = pow(saturate(1.0f - (distance / allLights.spotLights[k].distance)), allLights.spotLights[k].decay);
        float cosToPos = dot(-L, normalize(allLights.spotLights[k].direction));
        float spotFactor = saturate((cosToPos - allLights.spotLights[k].cosAngle) / (1.0f - allLights.spotLights[k].cosAngle));
        float intensity = allLights.spotLights[k].intensity * attenuation * spotFactor;

        float3 lightColor = allLights.spotLights[k].color.rgb;

        float roughness = saturate(material.roughness);
        float metallic = saturate(material.metallic);
        float3 albedo = material.color.rgb * color.rgb;
        float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

        float D = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = FresnelSchlick(saturate(dot(H, V)), F0);

        float3 specular = (D * G * F) / max(4.0f * saturate(dot(N, V)) * saturate(dot(N, L)), 0.01f);
        float3 kD = (float3(1.0f, 1.0f, 1.0f) - F) * (1.0f - metallic);
        float3 diffuse = kD * albedo / PI;

        totalDiffuse += (diffuse + specular) * lightColor * intensity * saturate(dot(N, L));
    }
    
    // RectLight
    for (int l = 0; l < allLights.count.rectLight; ++l)
    {
        float3 halfW = allLights.rectLights[l].right * (allLights.rectLights[l].size.x * 0.5f);
        float3 halfH = allLights.rectLights[l].up * (allLights.rectLights[l].size.y * 0.5f);
        
        float3 p[4];
        p[0] = allLights.rectLights[l].position - halfW - halfH; // 左下
        p[1] = allLights.rectLights[l].position + halfW - halfH; // 右下
        p[2] = allLights.rectLights[l].position + halfW + halfH; // 右上
        p[3] = allLights.rectLights[l].position - halfW + halfH; // 左上

        float3 v[4];
        for (int idx = 0; idx < 4; idx++)
        {
            v[idx] = normalize(p[idx] - worldPosition);
        }

        float illuminance = 0.0f;
        illuminance += acos(saturate(dot(v[0], v[1])));
        illuminance += acos(saturate(dot(v[1], v[2])));
        illuminance += acos(saturate(dot(v[2], v[3])));
        illuminance += acos(saturate(dot(v[3], v[0])));
        
        illuminance /= (2.0f * PI);

        float3 distVec = worldPosition - allLights.rectLights[l].position;
        float distance = length(distVec);
        float attenuation = pow(saturate(1.0f - (distance / 20.0f)), allLights.rectLights[l].decay);

        float3 N = normalize(worldNormal);
        float3 L = normalize(-allLights.rectLights[l].direction);
        float nDotL = saturate(dot(N, L));

        float3 albedo = material.color.rgb * color.rgb;
        float3 diffuse = (albedo / PI) * allLights.rectLights[l].color.rgb * allLights.rectLights[l].intensity * illuminance * nDotL * attenuation;

        totalDiffuse += diffuse;
    }
    
    // アルベドの計算
    float3 albedo = material.color.rgb * color.rgb;
    
    // 環境光の計算
    float3 ambient = allLights.ambientColor.rgb * albedo;
   
    // 直接光と環境光を合成
    return totalDiffuse + ambient;
}