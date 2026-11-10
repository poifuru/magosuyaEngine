#include "PCH.h"
#include "Vignette.h"
#include "imgui.h"
#include "WindowsAPI.h"

void Vignette::Initialize(ID3D12Device* device) {
	shadingModel_ = MyEngine::Rendering::ShadingModel::PostEffect_Vignette;

	// リソースの初期化
	buffer_ = std::make_unique<ConstantBuffer<VignetteForGPU>>();
	buffer_->Initialize(device);

	param_.centerAndRadius = { 0.5f, 0.5f, 0.8f, 1.f };
	param_.colorAndIntensity = { 0.0f, 0.0f, 0.0f, 1.0f };
	param_.aspectAndPadding = { WindowsAPI::GetInstance()->GetAspectRatio(), 0.0f, 0.0f, 0.0f };

	buffer_->Update(param_);
}

void Vignette::ImGui() {
	bool changed = false;
	changed |= ImGui::DragFloat2("中心##Vignette", &param_.centerAndRadius.x, 0.01f, 0.0f, 1.0f);
	changed |= ImGui::DragFloat("内側の半径##Vignette", &param_.centerAndRadius.z, 0.01f, 0.0f, 1.0f);
	changed |= ImGui::DragFloat("外側の半径##Vignette", &param_.centerAndRadius.w, 0.01f, 0.0f, 1.0f);
	changed |= ImGui::DragFloat3("色##Vignette", &param_.colorAndIntensity.x, 0.01f, 0.0f, 1.0f);
	changed |= ImGui::DragFloat("強さ##Vignette", &param_.colorAndIntensity.w, 0.01f, 0.0f, 1.0f);
	ImGui::Text("アスペクト比 : %.3f", param_.aspectAndPadding.x);

	// ImGuiで値が変わったら更新
	if (changed) {
		buffer_->Update(param_);
	}
}

const char* Vignette::GetDisplayName() const {
	return "ビネット";
}

D3D12_GPU_VIRTUAL_ADDRESS Vignette::GetConstantBufferAddress() const {
	return buffer_->GetGPUVirtualAddress();
}
