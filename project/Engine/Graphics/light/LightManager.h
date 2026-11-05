#pragma once
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "RectLight.h"
#include "ConstantBuffer.h"

// 前方宣言
class LightComponent;

//ライトの最大数
const uint32_t MaxCount = 50;

//ライトの種類
enum LightType {
	DIRECTIONALLIGHT,
	POINTLIGHT,
	SPOTLIGHT,
	RECTLIGHT,
};

struct LightCount {
	int32_t dirLight;
	int32_t pointLight;
	int32_t spotLight;
	int32_t rectLight;
};

struct DirectionalLightForGPU {
	Vector4 color;
	Vector3 direction;
	float intensity;
};

struct PointLightForGPU {
	Vector4 color;
	Vector3 position;
	float intensity;
	float radius;	//ライトが届く最大距離
	float decay;	//減衰率
	float padding[2];
};

struct SpotLightForGPU {
	Vector4 color;
	Vector3 position;
	float intensity;
	Vector3 direction;
	float distance;
	float decay;
	float cosAngle;
	float padding[2];
};

struct RectLightForGPU {
	Vector4 color;
	Vector3 position;    // ライトの中心座標
	float intensity;
	Vector3 direction;   // ライトの正面方向（法線）
	float padding1;
	Vector2 size;        // Width(幅) と Height(高さ)
	float padding2[2];
	Vector3 right;       // ライトの右方向ベクトル
	float padding;
	Vector3 up;          // ライトの上方向ベクトル
	float decay;        // 距離による減衰率（PointLightと同様）
};

// すべてのライトをまとめる構造体
struct AllLightDataForGPU {	
	Vector4 ambientColor;
	LightCount count;

	DirectionalLightForGPU dirLights[MaxCount];
	PointLightForGPU pointLights[MaxCount];
	SpotLightForGPU spotLights[MaxCount];
	RectLightForGPU rectLights[MaxCount];
};

class LightManager {
public:
	LightManager() = default;
	~LightManager() = default;

	void Initialize(ID3D12Device* device);
	void Update();

	void ImGui();

	// フレームの最初にライト数をゼロにリセットする
	void ClearLights();

	// ライトを登録する
	void Register(const LightComponent* light);

	// 描画時にバッファーのアドレスを渡すための関数
	D3D12_GPU_VIRTUAL_ADDRESS GetLightGPUAddress() const { return lightBuffer_.GetGPUVirtualAddress(); }

	// 影を落とす平行光源（1本目）の向きを取得する。平行光源が無ければ false
	bool GetMainDirectionalLightDirection(Vector3& outDirection) const {
		if (lightCPUData_.count.dirLight <= 0) return false;
		outDirection = lightCPUData_.dirLights[0].direction;
		return true;
	}

private:
	// ライト登録の補助関数
	void RegisterDirectionalLight(const Vector4& color, const Vector3& direction, float intensity);
	void RegisterPointLight(
		const Vector4& color, const Vector3& position, float intensity, float radius, float decay
	);
	void RegisterSpotLight(
		const Vector4& color, const Vector3& position, float intensity, 
		const Vector3& direction, float distance, float decay, float cosAngle
	);
	void RegisterRectLight(
		const Vector4& color, const Vector3& position, float intensity, const Vector3& direction, 
		const Vector2& size, const Vector3& right, const Vector3& up, float decay
	);

private:
	//ライトの種類ごとにリストを持つ
	std::vector<std::unique_ptr<DirectionalLight>> dirLights_;
	std::vector<std::unique_ptr<PointLight>> pointLights_;
	std::vector<std::unique_ptr<SpotLight>> spotLights_;
	std::vector<std::unique_ptr<RectLight>> rectLights_;

	ConstantBuffer<AllLightDataForGPU> lightBuffer_;
	AllLightDataForGPU lightCPUData_{}; // CPU側の一時保存用

	//ImGui編集用の変数
	int selectDirLightIndex_ = 0;
	int selectPointLightIndex_ = 0;
	int selectSpotLightIndex_ = 0;
	int selectRectLightIndex_ = 0;

	Vector3 ambientColor_ = { 0.2f, 0.3f, 0.45f }; // デフォルトはほんのり青みがかった空色
	float ambientIntensity_ = 5.0f;                // 環境光の強さ
};