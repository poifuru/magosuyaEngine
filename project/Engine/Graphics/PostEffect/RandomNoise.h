#pragma once
#include "BasePostEffect.h"
#include "ConstantBuffer.h"

class RandomNoise : public BasePostEffect {
public:
	void Initialize(ID3D12Device* device) override;
	void ImGui() override;
	const char* GetDisplayName() const override;
	D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferAddress() const override;

private:
	struct alignas(16) RandomNoiseForGPU {
		float time;
		float intensity;
		float padding[2];
	};

private:
	std::unique_ptr<ConstantBuffer<RandomNoiseForGPU>> buffer_;
	RandomNoiseForGPU param_;
};

