#pragma once
#include "ComputePipeline.h"

// 前方宣言
namespace MyEngine::Rendering {
	class RootSignatureManager;
}

// 船以外で波を立てるもの（ボーダー・着水・障害物など）
// 32バイト = float4 × 2。Ripple.CS.hlsl の WaveSource と並びを合わせる
struct WaveSource {
	Vector2 position;       // ワールドの (x, z)
	Vector2 velocity;       // 動いている速度 (x, z) [m/s]。船と同じ「押し＋排水」の波を立てる
	float radius = 1.5f;    // 影響半径 [m]
	float strength = 0.0f;  // 動きで立てる波の強さ（船 = 1）
	float impulse = 0.0f;   // その場で一度に押し込む深さ [m]（マイナスでへこませる。着水など1フレームだけ使う）
	float foam = 0.0f;      // 泡の量（そのフレームに足す濃さ）
};

// CSに送るパラメータ構造体
struct RippleSimulationParams {
	Vector3 boatPosition; // 船の位置
	float time;            // 経過時間

	Vector2 textureSize;   // 解像度 (512, 512)
	float deltaTime;       // デルタタイム
	float padding;

	Vector2 waterMin;
	Vector2 waterSize;

	uint32_t preTextureIndex;
	float speed;
	Vector2 forward;

	Vector4 iWaveKernel[43];

	Vector2 prevBoatPosition; // 前フレームの船の位置 (x, z)
	int scrollX;			  // 領域が動いたテクセル
	int scrollY;

	// 船以外の波源
	static constexpr uint32_t kMaxWaveSources = 16; // Ripple.CS.hlsl の kMaxWaveSources と合わせる
	uint32_t sourceCount;
	float padding2[3];
	WaveSource sources[kMaxWaveSources];
};

class RippleSimulator {
public:
	RippleSimulator() = default;
	~RippleSimulator() = default;

	// 初期化
	void Initialize(
		ID3D12Device* device,
		MyEngine::LowLevel::SrvDescriptorHeapPool& heapManager,
		MyEngine::Rendering::ShaderManager& shaderManager,
		MyEngine::Rendering::RootSignatureManager& rootSigManager,
		uint32_t width = 512,
		uint32_t height = 512
	);

	// CS起動
	void Dispatch(
		ID3D12GraphicsCommandList* cmdList,
		const Vector3& boatPos,
		float speed,
		float deltaTime,
		const Vector2& waterMin, 
		const Vector2& waterSize,
		const Vector2& forward,
		Vector4 kernel[43],
		int scrollX,
		int scrollY,
		const WaveSource* sources = nullptr,
		uint32_t sourceCount = 0
	);

	// 作成されたテクスチャのバインドレス SRV インデックスを取得
	uint32_t GetTextureSrvIndex() const { return textureIndex_; }

private:
	ComputePipeline<RippleSimulationParams> prePipeline_;
	ComputePipeline<RippleSimulationParams> currentPipeline_;

	int currentIndex_ = 0; // 今どっちを実行するかのフラグ (0 か 1)

	// 実際に渡すためのテクスチャインデックス
	uint32_t textureIndex_;

	// 前フレームの船の位置 (x, z)
	Vector2 prevBoatPos_{};
	bool hasPrevBoatPos_ = false; // 初回フレームかどうか
};