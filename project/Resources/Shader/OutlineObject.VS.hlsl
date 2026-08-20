struct VSInput {
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal   : NORMAL;
};

struct VSOutput {
    float4 position : SV_POSITION;
};

cbuffer TransformBuffer : register(b0) {
    matrix g_World;
    matrix g_WVP;
    matrix g_WorldInverseTranspose;
};

VSOutput main(VSInput input) {
    VSOutput output;

    // アウトラインの線の太さ（膨らませる幅）
    float outlineWidth = 0.08f; 

    // 法線方向に頂点を少し外側へ移動
    float3 expandedPos = input.position.xyz + input.normal * outlineWidth;

    // WVP行列で画面座標に変換
    output.position = mul(float4(expandedPos, 1.0f), g_WVP);
    return output;
}
