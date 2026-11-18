#pragma once
#include "GpuParticlePipelines.h"

// 「1秒あたり rate 個」を、毎フレームの個数に直す（端数は次のフレームへ持ち越す）
// 出し続けるエフェクトを使う側（コンポーネントやエディタのプレビュー）が1つずつ持つ
struct ParticleEmitTimer {
	float accumulator = 0.0f;

	uint32_t Step(float rate, float deltaTime) {
		accumulator += rate * deltaTime;
		uint32_t count = static_cast<uint32_t>(accumulator);
		accumulator -= static_cast<float>(count);
		return count;
	}
};

namespace MyEngine::LowLevel {
	class SrvDescriptorHeapPool;
}

namespace MyEngine::Rendering {
	class ShaderManager;
	class PSOManager;
	class InputLayoutManager;
	class BlendModeManager;
}

class TextureManager;
class GpuParticlePool;
struct CameraData;
struct ParticleEffectData;

class GpuParticleSystem {
public:
	GpuParticleSystem(
		ID3D12Device* device,
		MyEngine::Rendering::ShaderManager& shaderManager,
		ID3D12RootSignature* particleComputeRootSig,
		MyEngine::Rendering::PSOManager& psoManager,
		const MyEngine::Rendering::InputLayoutManager& inputLayoutManager,
		const MyEngine::Rendering::BlendModeManager& blendModeManager,
		ID3D12RootSignature* commonRootSig,
		MyEngine::LowLevel::SrvDescriptorHeapPool& heap
	);

	// Resources/Particle/*.json を全部読む
	void LoadAll(TextureManager& textureManager);

	// ゲームから呼ぶ
	// このフレームに position から count 個出す（出し続けるときは毎フレーム呼ぶ）
	void Emit(const std::string& name, const Vector3& position, uint32_t count);

	// position から、そのエフェクトの burstCount 個を一度に出す
	void Burst(const std::string& name, const Vector3& position);

	// Renderer から呼ぶ
	void Dispatch(
		ID3D12GraphicsCommandList* cmdList,
		D3D12_GPU_DESCRIPTOR_HANDLE srvTableStart,
		float deltaTime
	);

	void Draw(
		ID3D12GraphicsCommandList* cmdList,
		ID3D12RootSignature* commonRootSig,
		D3D12_GPU_DESCRIPTOR_HANDLE srvTableStart,
		const CameraData& camera
	);

	// エディタ用
	GpuParticlePool* FindPool(const std::string& name);
	std::vector<std::string> GetEffectNames() const;           // 一覧表示用（名前順）
	ParticleEffectData* FindEffect(const std::string& name);   // 編集する元データ
	void ApplyEffect(const std::string& name);                 // effects_[name] を Pool に反映
	bool SaveEffect(const std::string& name) const;            // Resources/Particle/<name>.json に書き出す
	bool CreateEffect(const std::string& name);                // デフォルト値で新しく作って保存する
	uint32_t GetTextureIndex(const std::string& path) const;   // 読み込み済みテクスチャの番号
	const std::vector<std::string>& GetTexturePaths() const { return texturePaths_; }   // エディタの選択肢用

	const GpuParticlePipelines& GetPipelines() const { return pipelines_; }

private:
	ID3D12Device* device_ = nullptr;
	MyEngine::LowLevel::SrvDescriptorHeapPool* heap_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> initPso_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> emitPso_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> updatePso_;
	GpuParticlePipelines pipelines_ = {};

	std::map<std::string, std::unique_ptr<GpuParticlePool>> pools_;   // 名前 → プール
	std::map<std::string, ParticleEffectData> effects_;               // 元データ（エディタで編集・保存する）
	std::map<std::string, uint32_t> textures_;                        // テクスチャのパス → 番号（読み込み済みのもの）
	std::vector<std::string> texturePaths_;                           // 読み込んだ順のパス（エディタの Combo 用）
};