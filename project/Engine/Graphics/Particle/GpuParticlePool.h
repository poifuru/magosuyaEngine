#pragma once
#include "ConstantBuffer.h"
#include "ParticleInfo.h"

namespace MyEngine::LowLevel {
	class SrvDescriptorHeapPool;
}

struct CameraData;
struct GpuParticlePipelines;
struct ParticleEffectData;

// 放出の要求（このフレームに、この位置へ、この数）
struct EmitRequest {
	Vector3 position;
	uint32_t count;
};

class GpuParticlePool {
public:
	GpuParticlePool(
		ID3D12Device* device,
		const GpuParticlePipelines& pipelines,    // System から借りる（Pool は消さない）
		const ParticleEffectData& data,
		uint32_t textureIndex,
		MyEngine::LowLevel::SrvDescriptorHeapPool& heap
	);

	// Renderer::DispatchCS から呼ぶ（今は初回だけ Initialize CS を流す）
	void Dispatch(
		ID3D12GraphicsCommandList* cmdList,
		D3D12_GPU_DESCRIPTOR_HANDLE srvTableStart,
		float deltaTime
		);

	void Draw(
		ID3D12GraphicsCommandList* cmdList,
		ID3D12RootSignature* commonRootSig,
		D3D12_GPU_DESCRIPTOR_HANDLE srvTableStart,
		const CameraData& camera,
		uint32_t drawSlot = 0
	);

	// 見た目・動きの設定を入れ直す
	void ApplyEffect(const ParticleEffectData& data);

	uint32_t GetMaxCount() const { return maxCount_; }

	// 放出の要求を積む。Dispatch でまとめて流し、流したら空になる
	void RequestEmit(const Vector3& position, uint32_t count);
	uint32_t GetBurstCount() const { return burstCount_; }

	// デバッグ用：頂上カウンタ（1フレーム遅れ）
	int32_t GetFreeListTop();

	static int32_t GetDebugFreeListTop() { return sDebugFreeListTop_; }

	void SetTextureIndex(uint32_t index) { textureIndex_ = index; }

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateUAVBuffer(ID3D12Device* device, UINT64 size);
	void ReadBackIfNeeded();

private:
	static constexpr uint32_t kMaxEmitPerFrame = 32;   // 1エフェクトあたり、1フレームに受け付ける要求の数

	Microsoft::WRL::ComPtr<ID3D12Resource> particleBuffer_;  // u0
	Microsoft::WRL::ComPtr<ID3D12Resource> freeListBuffer_;  // u1
	Microsoft::WRL::ComPtr<ID3D12Resource> freeListTopBuffer_; // u2
	Microsoft::WRL::ComPtr<ID3D12Resource> readbackBuffer_;  // 頂上カウンタの確認用
	ConstantBuffer<EmitterForGPU> emitterBuffer_;

	const GpuParticlePipelines* pipelines_ = nullptr;   // System から借りる

	uint32_t maxCount_ = 0;
	bool needsInitialize_ = true;
	bool hasPendingCopy_ = false;
	int32_t freeListTop_ = -1;

	static inline int32_t sDebugFreeListTop_ = -1;
	
	EmitterForGPU emitter_{};   // CPU側の写し（毎フレーム書き換えて Update する）
	uint32_t frameCount_ = 0;   // 乱数の種用

	uint32_t particleSrvIndex_ = 0;
	static constexpr uint32_t kDrawSlotCount = 2;   // 0: ゲーム画面、1: エディタのプレビュー
	ConstantBuffer<ParticleDrawForGpu> drawBuffers_[kDrawSlotCount];
	ID3D12PipelineState* drawPso_ = nullptr;

	uint32_t textureIndex_ = 0;

	ConstantBuffer<EmitterForGPU> emitBuffers_[kMaxEmitPerFrame];   // 要求ごとに別の定数バッファを使う
	std::vector<EmitRequest> emitRequests_;                          // このフレームに来た放出の要求
	uint32_t burstCount_ = 0;  
};