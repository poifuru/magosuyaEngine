#include "PCH.h"
#include "Fog.h"

void Fog::Initialize(ID3D12Device* device) {
	shadingModel_ = MyEngine::Rendering::ShadingModel::PostEffect_Fog;

	// リソースの初期化
	buffer_ = std::make_unique<ConstantBuffer<FogForGPU>>();
	buffer_->Initialize(device);

	param_.color = { 0.0f, 0.782f, 1.0f, 0.9f };
	param_.start = 50.0f;
	param_.end = 80.0f;
	param_.cameraNear = 0.1f;
	param_.cameraFar = 1000.0f;
	param_.heightStart = -20.0f;
	param_.heightEnd = -50.0f;

	// 初期データを定数バッファに転送
	buffer_->Update(param_);
}

void Fog::ImGui() {
	bool changed = false;
	changed |= ImGui::ColorEdit4("色##Fog", &param_.color.x);
	changed |= ImGui::DragFloat("開始距離##Fog", &param_.start, 0.1f, 0.0f, 1000.0f);
	changed |= ImGui::DragFloat("終了距離##Fog", &param_.end, 0.1f, 0.0f, 1000.0f);
	changed |= ImGui::DragFloat("高さフォグの開始##Fog", &param_.heightStart, 0.1f, -100.0f, 100.0f);
	changed |= ImGui::DragFloat("高さフォグの終了##Fog", &param_.heightEnd, 0.1f, -100.0f, 100.0f);
	ImGui::Text("カメラの近クリップ : %.3f", param_.cameraNear);
	ImGui::Text("カメラの遠クリップ : %.3f", param_.cameraFar);

	// ImGuiで値が変わったら更新
	if (changed) {
		buffer_->Update(param_);
	}
}

D3D12_GPU_VIRTUAL_ADDRESS Fog::GetConstantBufferAddress() const {
    return buffer_->GetGPUVirtualAddress();
}
