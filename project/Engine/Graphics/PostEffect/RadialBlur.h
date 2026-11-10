#pragma once
#include "BasePostEffect.h"
#include "ConstantBuffer.h"

class RadialBlur : public BasePostEffect {
public:
	void Initialize(ID3D12Device* device) override;
	void ImGui() override;
	const char* GetDisplayName() const override;
	D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferAddress() const override;

private:
	struct alignas(16) RadialBlurForGPU {
		Vector2 center;
		float blurWidth;
		int sampleCount;
	};

private:
	std::unique_ptr<ConstantBuffer<RadialBlurForGPU>> buffer_;
	RadialBlurForGPU param_;
};

