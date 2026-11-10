#pragma once
#include "BasePostEffect.h"
#include "ConstantBuffer.h"

class Vignette : public BasePostEffect {
public:
	void Initialize(ID3D12Device* device) override;

	void ImGui() override;

	const char* GetDisplayName() const override;

	D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferAddress() const override;

private:
	struct alignas(16) VignetteForGPU {
		Vector4 centerAndRadius;      // xy: center, z: innerRadius, w: outerRadius
		Vector4 colorAndIntensity;    // xyz: color, w: intensity
		Vector4 aspectAndPadding;     // x: aspectRatio, yzw: padding
	};

private:
	std::unique_ptr<ConstantBuffer<VignetteForGPU>> buffer_;
	VignetteForGPU param_;
};