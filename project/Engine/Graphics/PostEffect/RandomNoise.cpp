#include "PCH.h"
#include "RandomNoise.h"

void RandomNoise::Initialize(ID3D12Device* device) {
	shadingModel_ = MyEngine::Rendering::ShadingModel::PostEffect_RandomNoise;

	// リソースの初期化
	buffer_ = std::make_unique<ConstantBuffer<RandomNoiseForGPU>>();
	buffer_->Initialize(device);

	param_.time = 0.0f;
	param_.intensity = 0.15f;

	buffer_->Update(param_);
}

void RandomNoise::ImGui() {
	bool changed = false;
	changed |= ImGui::DragFloat("強さ##Noise", &param_.intensity, 0.005f, 0.0f, 1.0f, "%.3f");

	// ImGuiで値が変わったら更新
	if (changed) {
		buffer_->Update(param_);
	}
}

D3D12_GPU_VIRTUAL_ADDRESS RandomNoise::GetConstantBufferAddress() const {
	return buffer_->GetGPUVirtualAddress();
}
