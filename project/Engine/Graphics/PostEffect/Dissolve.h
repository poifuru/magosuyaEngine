#pragma once
#include "BasePostEffect.h"
#include "ConstantBuffer.h"

class Dissolve : public BasePostEffect {
public:
	void Initialize(ID3D12Device* device) override;
	void ImGui() override;
	D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferAddress() const override;
	uint32_t GetExtraSrvIndex() const override { return maskTextureSrvIndex_; }

private:
	struct alignas(16) DissolveForGPU {
		Vector4 edgeColor;
		float threshold;
		float edgeWidth;
		float padding[2];
	};

private:
	std::unique_ptr<ConstantBuffer<DissolveForGPU>> buffer_;
	DissolveForGPU param_;

	// マスクテクスチャのインデックス
	uint32_t maskTextureSrvIndex_ = 0;
};