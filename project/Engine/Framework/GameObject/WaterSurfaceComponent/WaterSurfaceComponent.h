#pragma once
#include "RenderComponent.h"
#include "ConstantBuffer.h"
#include "MeshData.h"
#include "Material.h"

namespace MyEngine::Rendering {
	class RenderCollector;
}

class WaterSurfaceComponent : public MyEngine::Rendering::RenderComponent {
public:
	static constexpr int kMaxBaseWaves = 6; // インスペクターで作る「普段の波」の最大本数
	static constexpr int kStormWaves = 2;   // 「荒れ用の波（うねり）」の本数。RoughSeaComponent が強さを変える
	static constexpr int kMaxWaves = kMaxBaseWaves + kStormWaves; // GPU に送る最大本数（HLSL の kMaxWaves と揃える）

	void Initialize() override;

	void Update() override;

	void ImGui() override;

	float SampleGerstnerHeight(float x, float z) const;

	void SampleGerstnerSlope(float x, float z, float& dhdx, float& dhdz) const;

	// ===== 荒れ用の波（うねり） =====
	// 強さ 0〜1。荒れ用の波の「振幅」と「とがり具合」の両方に掛かる
	// （Q = とがり / (k·A·本数) なので、振幅だけ 0 にしても横のずれ Q·A が残るため）
	void SetStormWeight(float weight);
	float GetStormWeight() const { return stormWeight_; }
	// うねりの進む向き（XZ）。位相 k·(D·x) がずれて波が瞬間移動しないよう、強さが 0 のときだけ変えられる
	// 戻り値：変えられたら true
	bool SetStormDirection(const Vector2& dir);
	// 荒れ用の波 index の波数 k（周波数）。波長 = 2π / k、進む速さ = √(g / k)
	float GetStormWaveNumber(int index) const;

	// 描画素材収集関数のオーバーライド
	void CollectRenderData(MyEngine::Rendering::RenderCollector* collector) const override;

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

	// 水面メッシュをカメラの真下へ動かす（頂点の間隔にスナップ）
	void FollowCamera();

	// GPU と CPU（SampleGerstnerHeight）で使う波の一覧を作り直す（普段の波 + 荒れ用の波）
	void RebuildActiveWaves();

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
		WaterSurfaceInfo waves[kMaxWaves];
		float time;
		int numActiveWaves;
		float nearFadeDistance;
		float farFadeDistance;
		uint32_t rippleTextureIndex;
		float shadowStrength;			// 影の中の水の色をどれだけ暗くするか（0〜1）
		float shadowReflectionStrength;	// 影の中の空の反射をどれだけ暗くするか（0〜1）
		float padding;
		Vector2 waterMin; 
		Vector2 waterSize;
	};
	static_assert(sizeof(WaterSurfaceForGPU) == 32 * kMaxWaves + 48, "WaterSurfaceForGPU と HLSL の WaterSurface がずれている");

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

	// メッシュの作り直しが必要かどうか
	// AddComponent時のInitializeと直後のDeserializeで二重にメッシュを作らないよう、実際の生成は次のUpdateまで遅らせる
	bool isMeshDirty_ = true;

	// カメラ追従
	// 波はすべてワールド座標で計算しているので、メッシュを動かしても波の形は変わらない
	bool followCamera_ = true;

	float time_ = 0.0f;

	// ゲルストナー波のパラメータ
	WaterSurfaceInfo waves_[kMaxWaves];
	int numActiveWaves_ = 2; // デフォルトで有効にする波の数（1〜kMaxBaseWaves）

	// 荒れ用の波（シリアライズ対象。向きは stormDir_ から angle だけ回した向き）
	struct StormWave {
		float amplitude;  // 振幅[m]
		float frequency;  // 波数 k（波長 = 2π / k）
		float steepness;  // とがり具合
		float angle;      // うねりの向きからのずれ[rad]
	};
	StormWave stormWaves_[kStormWaves] = {
		{ 1.2f, 0.05f, 0.4f, 0.0f },  // 本体：波長 約126 m
		{ 0.4f, 0.09f, 0.3f, 0.35f }, // 添え：波長 約70 m、少し斜め（同じ形の繰り返しに見えないように）
	};

	// 荒れ用の波の実行時の状態（シリアライズしない）
	float stormWeight_ = 0.0f;
	Vector2 stormDir_ = { 0.0f, 1.0f };

	// 実際に使う波（普段の波 + 荒れ用の波）。GPU にも CPU のサンプルにもこれを使う
	WaterSurfaceInfo activeWaves_[kMaxWaves]{};
	int activeCount_ = 0;
	float nearFadeDistance_ = 30.0f; // デフォルト値（近距離で透け始める距離）
	float farFadeDistance_ = 150.0f; // デフォルト値（これより遠いと完全に不透明）

	// 水面に落ちる影の濃さ（0 で影なし、1 で真っ暗）
	float shadowStrength_ = 0.8f;			// 水の中の色
	float shadowReflectionStrength_ = 0.5f;	// 空の反射

	ConstantBuffer<WaterSurfaceForGPU> waterSurfaceBuffer_;
};