#include "PCH.h"
#include "ColorGrading.h"
#include "ColorGrading.h"
#include "imgui.h"

void ColorGrading::Initialize(ID3D12Device* device) {
	shadingModel_ = MyEngine::Rendering::ShadingModel::PostEffect_ColorGrading;

	// リソースの初期化
	buffer_ = std::make_unique<ConstantBuffer<ColorGradingForGPU>>();
	buffer_->Initialize(device);

	param_.intensity = 1.0f;
	param_.sepiaAmount = 0.0f;
	param_.tintColor = { 1.0f, 0.95f, 0.82f };

	// 初期データを定数バッファに転送
	buffer_->Update(param_);
}

void ColorGrading::ImGui() {
	bool changed = false;
	changed |= ImGui::DragFloat("強さ##ColorGrading", &param_.intensity, 0.01f, 0.0f, 1.0f);
	changed |= ImGui::DragFloat("セピアの量##ColorGrading", &param_.sepiaAmount, 0.01f, 0.0f, 1.0f);
	changed |= ImGui::DragFloat3("色味##ColorGrading", &param_.tintColor.x, 0.01f, 0.0f, 1.0f);

	// ImGuiで値が変わったら更新
	if (changed) {
		buffer_->Update(param_);
	}
}

D3D12_GPU_VIRTUAL_ADDRESS ColorGrading::GetConstantBufferAddress() const {
	return buffer_->GetGPUVirtualAddress();
}