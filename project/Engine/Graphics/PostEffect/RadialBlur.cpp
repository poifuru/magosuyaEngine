#include "PCH.h"
#include "RadialBlur.h"

void RadialBlur::Initialize(ID3D12Device* device) {
	shadingModel_ = MyEngine::Rendering::ShadingModel::PostEffect_RadialBlur;

	// リソースの初期化
	buffer_ = std::make_unique<ConstantBuffer<RadialBlurForGPU>>();
	buffer_->Initialize(device);

	param_.center = { 0.5f, 0.5f };
	param_.blurWidth = 0.01f;
	param_.sampleCount = 10;

	buffer_->Update(param_);
}

void RadialBlur::ImGui() {
	bool changed = false;
	changed |= ImGui::DragFloat2("中心##RadialBlur", &param_.center.x, 0.01f, 0.0f, 1.0f);
	changed |= ImGui::DragFloat("ぼかしの幅##RadialBlur", &param_.blurWidth, 0.001f, 0.0f, 0.1f);
	changed |= ImGui::DragInt("サンプル数##RadialBlur", &param_.sampleCount, 1.f, 1, 64);

	// ImGuiで値が変わったら更新
	if (changed) {
		buffer_->Update(param_);
	}
}

D3D12_GPU_VIRTUAL_ADDRESS RadialBlur::GetConstantBufferAddress() const {
	return buffer_->GetGPUVirtualAddress();
}
