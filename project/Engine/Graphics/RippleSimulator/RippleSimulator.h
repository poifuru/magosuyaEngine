#pragma once
#include "ComputePipeline.h"

// 前方宣言
namespace MyEngine::Rendering {
	class RootSignatureManager;
}

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

	void Dispatch(
		ID3D12GraphicsCommandList* cmdList,
		const Vector3& boatPos,
		float speed,
		float deltaTime,
		const Vector2& waterMin, 
		const Vector2& waterSize,
		const Vector2& forward
	);

	// 作成されたテクスチャのバインドレス SRV インデックスを取得
	uint32_t GetTextureSrvIndex() const { return textureIndex_; }

private:
	ComputePipeline<RippleSimulationParams> prePipeline_;
	ComputePipeline<RippleSimulationParams> currentPipeline_;

	int currentIndex_ = 0; // 今どっちを実行するかのフラグ (0 か 1)

	// 実際に渡すためのテクスチャインデックス
	uint32_t textureIndex_;
};