#pragma once
#include "ConstantBuffer.h"
#include "ParticleInfo.h"

namespace MyEngine::LowLevel {
	class SrvDescriptorHeapPool;
}

struct CameraData;
struct GpuParticlePipelines;
struct ParticleEffectData;

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
		const CameraData& camera
	);

	// 見た目・動きの設定を入れ直す
	void ApplyEffect(const ParticleEffectData& data);

	void SetPosition(const Vector3& position) { emitter_.position = position; }

	void RequestBurst(const Vector3& position);

	// デバッグ用：頂上カウンタ（1フレーム遅れ）
	int32_t GetFreeListTop();

	static int32_t GetDebugFreeListTop() { return sDebugFreeListTop_; }

	void SetTextureIndex(uint32_t index) { textureIndex_ = index; }

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateUAVBuffer(ID3D12Device* device, UINT64 size);
	void ReadBackIfNeeded();

private:
	static constexpr uint32_t kMaxBurstPerFrame = 8;

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
	ConstantBuffer<ParticleDrawForGpu> drawBuffer_;
	ID3D12PipelineState* drawPso_ = nullptr;

	float emitRate_ = 300.0f;      // 1秒あたりの放出数
	float emitAccumulator_ = 0.0f; // 端数の持ち越し

	uint32_t textureIndex_ = 0;

	ConstantBuffer<EmitterForGPU> burstBuffers_[kMaxBurstPerFrame];
	std::vector<Vector3> burstRequests_;                               // このフレームに来た Burst の位置
	uint32_t burstCount_ = 0;  
};