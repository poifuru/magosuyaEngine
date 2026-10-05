#pragma once
#include "Component.h"
#include "ConstantBuffer.h"
#include "MeshData.h"
#include "Material.h"

class WaterSurfaceComponent : public Component {
public:
	void Initialize() override;

	void Update() override;

	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "Water Surface"; }

	MyEngine::Rendering::StaticMesh* GetMesh() const { return mesh_.get(); }

	MyEngine::Rendering::Material* GetMaterial() const { return material_.get(); }

	void SetTexture(const std::string& textureName);

	// 水面のワールド範囲 (Min) を取得
	Vector2 GetWaterMin() const;

	// 水面のサイズ (Size) を取得
	Vector2 GetWaterSize() const;

	// カスタムバッファを取得
	D3D12_GPU_VIRTUAL_ADDRESS GetCustomBufferAddress() const {
		return waterSurfaceBuffer_.GetGPUVirtualAddress();
	}

private:
	void GenerateMesh();

private:
	// 波のパラメータ
	struct WaterSurfaceInfo {
		float amplitude;				// 振幅(A)
		float frequency;				// 周波数(w)
		float steepness;				// 険しさ(Q)
		float padding;
		Vector2 direction;				// 波の進行方向(D)
		float padding2[2];    
	};

	// シェーダーに送るパラメータ
	struct WaterSurfaceForGPU {
		WaterSurfaceInfo waves[4];
		float time;
		int numActiveWaves;
		float nearFadeDistance;
		float farFadeDistance;
		uint32_t rippleTextureIndex;
		float padding[3];
		Vector2 waterMin; 
		Vector2 waterSize;
	};

private:
	// メッシュとマテリアルを直接持たせる
	std::unique_ptr<MyEngine::Rendering::StaticMesh> mesh_;
	std::unique_ptr<MyEngine::Rendering::Material> material_;
	std::string texPath_ = "white1x1";		// デフォルトテクスチャ
	uint32_t texIndex_ = 0;

	// メッシュ用のパラメータ
	float width_ = 500.0f;
	float depth_ = 500.0f;
	int subdivisionX_ = 1000;
	int subdivisionZ_ = 1000;

	float time_ = 0.0f;

	// ゲルストナー波のパラメータ
	WaterSurfaceInfo waves_[4];
	int numActiveWaves_ = 2; // デフォルトで有効にする波の数（1〜4）
	float nearFadeDistance_ = 30.0f; // デフォルト値（近距離で透け始める距離）
	float farFadeDistance_ = 150.0f; // デフォルト値（これより遠いと完全に不透明）

	ConstantBuffer<WaterSurfaceForGPU> waterSurfaceBuffer_;
};