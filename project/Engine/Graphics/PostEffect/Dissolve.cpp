#include "PCH.h"
#include "Dissolve.h"

void Dissolve::Initialize(ID3D12Device* device) {
	shadingModel_ = MyEngine::Rendering::ShadingModel::PostEffect_Dissolve;

	// リソースの初期化
	buffer_ = std::make_unique<ConstantBuffer<DissolveForGPU>>();
	buffer_->Initialize(device);

	param_.edgeColor = { 1.0f, 0.5f, 0.0f, 1.0f }; // オレンジ色のエッジ
	param_.threshold = 0.0f;                       // 閾値初期値
	param_.edgeWidth = 0.02f;                      // エッジ幅初期値

	// 初期データを定数バッファに転送
	buffer_->Update(param_);
}

void Dissolve::ImGui() {
	bool changed = false;
	changed |= ImGui::DragFloat("threshold##Dissolve", &param_.threshold, 0.01f, 0.0f, 1.0f);
	changed |= ImGui::DragFloat("edgeWidth##Dissolve", &param_.edgeWidth, 0.005f, 0.0f, 0.2f);
	changed |= ImGui::ColorEdit3("edgeColor##Dissolve", &param_.edgeColor.x);

	// ImGuiで値が変わったら更新
	if (changed) {
		buffer_->Update(param_);
	}
}

D3D12_GPU_VIRTUAL_ADDRESS Dissolve::GetConstantBufferAddress() const {
	return buffer_->GetGPUVirtualAddress();
}

void Dissolve::SetThreshold(float threshold) {
	param_.threshold = threshold;
	if (buffer_) {
		buffer_->Update(param_);
	}
}

void Dissolve::SetEdgeColor(const Vector4& color) {
	param_.edgeColor = color;
	if (buffer_) {
		buffer_->Update(param_);
	}
}

void Dissolve::SetEdgeWidth(float width) {
	param_.edgeWidth = width;
	if (buffer_) {
		buffer_->Update(param_);
	}
}