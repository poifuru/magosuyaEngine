#pragma once
#include "BasePostEffect.h"
#include "ConstantBuffer.h"

class ColorGrading : public BasePostEffect {
public:
	void Initialize(ID3D12Device* device) override;

	void ImGui() override;

	const char* GetDisplayName() const override;

	D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferAddress() const override;

private:
	struct alignas(16) ColorGradingForGPU {
		float intensity = 1.0f;		// エフェクトの強度(0.0 : 通常, 1.0 : 完全な白黒)
		float sepiaAmount = 0.0f;	// セピア調にする強さ(0.0 : 完全な白黒, 1.0 : セピア)

		Vector3 tintColor {			// 画面に着色したい色(デフォルトはセピア調の RGB : 1.0, 0.95, 0.82)
			1.0f, 0.95f, 0.82f
		};
	};

private:
	std::unique_ptr<ConstantBuffer<ColorGradingForGPU>> buffer_;
	ColorGradingForGPU param_;
};