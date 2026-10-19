#pragma once
#include "RenderComponent.h"
#include "MeshData.h"
#include "Material.h"
#include "TransformMatrixData.h"
#include "RippleSimulator.h"

namespace MyEngine::Rendering {
	class RenderCollector;
}

// 航跡の制御点のデータ
struct WakePoint {
	Vector3 position;
	Vector3 rightDir;
	float age = 0.0f;
};

// シェーダーに送る頂点データ
struct WakeVertex {
	Vector3 position;
	float alpha;
	Vector2 uv;
};

class BoatWakeComponent : public MyEngine::Rendering::RenderComponent {
public:
	BoatWakeComponent() = default;
	~BoatWakeComponent() override = default;

	void Initialize() override;
	void Update() override;
	void ImGui() override;

	// 描画素材収集関数のオーバーライド
	void CollectRenderData(MyEngine::Rendering::RenderCollector* collector) const override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "Boat Wake"; }

	// CSを実行する関数
	void DispatchCS(ID3D12GraphicsCommandList* cmdList);

	void SetTexture(const std::string& textureName);

	Vector2 GetSimMin() const { return simMin_; }
	Vector2 GetSimSize() const { return Vector2(kSimSize, kSimSize); }

	// Renderer用
	const MyEngine::Rendering::Mesh& GetMesh() const { return mesh_; }
	MyEngine::Rendering::Material* GetMaterial() const { return material_.get(); }
	uint32_t GetTextureIndex() const { return texIndex_; }
	uint32_t GetRippleTextureIndex() const { return rippleSim_.GetTextureSrvIndex(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetTransformAddress() const { return transformBuffer_.GetGPUVirtualAddress(); }

private:
	// メッシュ構築処理
	void GenerateMesh();

	// iWave用のカーネルを構築する
	void BuildIWaveKernel();

private:
	RippleSimulator rippleSim_;		// 波紋シミュレーター
	Vector3 lastPos_;

	std::vector<WakePoint> points_;
	std::vector<WakeVertex> vertices_;

	// 動的頂点バッファ
	MyEngine::Rendering::GenericMesh<WakeVertex> mesh_;
	std::unique_ptr<MyEngine::Rendering::Material> material_;

	// 行列用の定数バッファ
	ConstantBuffer<TransformMatrixData> transformBuffer_;

	// パラメータ
	std::string texPath_ = "white1x1";		// デフォルトテクスチャ
	uint32_t texIndex_ = 0;
	float spawnInterval_ = 0.5f; // ポイント追加の移動距離（メートル）
	float maxLifetime_ = 4.0f;   // 泡が完全に消えるまでの時間（秒）
	float trailWidth_ = 1.5f;    // 船直後の航跡の基本幅
	float widthExpandRate_ = 1.5f; // 後方へのV字拡散率
	Vector3 lastSpawnPosition_{ 0.0f, 0.0f, 0.0f };
	Vector2 smoothedVelocity_{};
	Vector2 velocity;

	static constexpr size_t kMaxVertices = 1000; // 確保しておく最大頂点数

	// iWave用のカーネル
	Vector4 kernel_[43]{};

	// 波紋シミュレーションの領域（船の周り）
	static constexpr float kSimSize = 80.0f;   // 領域の一辺[m]
	static constexpr int   kSimRes  = 512;     // テクスチャ解像度
	int32_t originTexelX_ = 0;
	int32_t originTexelZ_ = 0;
	bool hasOrigin_ = false;
	Vector2 simMin_{};
};