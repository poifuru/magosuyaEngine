#pragma once
#include "BasePostEffect.h"

class CopyImageEffect : public BasePostEffect {
public:
	void Initialize(ID3D12Device* device) override;
	void ImGui() override;
	const char* GetDisplayName() const override;
	D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferAddress() const override;
};
