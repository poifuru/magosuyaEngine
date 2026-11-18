#pragma once
#include "GpuParticlePipelines.h"

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
	void Burst(const std::string& name, const Vector3& position);

	void SetPosition(const std::string& name, const Vector3& position);

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
};