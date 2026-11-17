#pragma once
#include "ConstantBuffer.h"
#include "ParticleInfo.h"

namespace MyEngine::Rendering {
	class ShaderManager;
}

class GpuParticlePool {
public:
	void Initialize(
		ID3D12Device* device,
		MyEngine::Rendering::ShaderManager& shaderManager,
		ID3D12RootSignature* particleComputeRootSig,
		uint32_t maxCount
	);

	// Renderer::DispatchCS から呼ぶ（今は初回だけ Initialize CS を流す）
	void Dispatch(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srvTableStart);

	// デバッグ用：頂上カウンタ（1フレーム遅れ）
	int32_t GetFreeListTop();

	static int32_t GetDebugFreeListTop() { return sDebugFreeListTop_; }

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateUAVBuffer(ID3D12Device* device, UINT64 size);
	void ReadBackIfNeeded();

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> particleBuffer_;  // u0
	Microsoft::WRL::ComPtr<ID3D12Resource> freeListBuffer_;  // u1
	Microsoft::WRL::ComPtr<ID3D12Resource> freeListTopBuffer_; // u2
	Microsoft::WRL::ComPtr<ID3D12Resource> readbackBuffer_;  // 頂上カウンタの確認用

	Microsoft::WRL::ComPtr<ID3D12PipelineState> initPso_;
	ID3D12RootSignature* rootSig_ = nullptr;
	ConstantBuffer<EmitterForGPU> emitterBuffer_;  // ステップ1では maxCount を渡すだけ

	uint32_t maxCount_ = 0;
	bool needsInitialize_ = true;
	bool hasPendingCopy_ = false;
	int32_t freeListTop_ = -1;

	static inline int32_t sDebugFreeListTop_ = -1;
};