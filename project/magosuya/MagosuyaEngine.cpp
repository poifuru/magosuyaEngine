#include "MagosuyaEngine.h"

MagosuyaEngine::MagosuyaEngine () {
	dxCommon_ = std::make_unique<DxCommon> ();
	textureManager_ = std::make_unique<TextureManager> (dxCommon_.get ());
	imGuiManager_ = std::make_unique<ImGuiManager> (dxCommon_.get ());
}

MagosuyaEngine::~MagosuyaEngine () {

}

void MagosuyaEngine::Initialize () {
	dxCommon_->Initialize ();
	textureManager_->Initialize ();
	imGuiManager_->Initialize ();
}

void MagosuyaEngine::BeginFrame () {
	imGuiManager_->BeginFrame ();
	dxCommon_->BeginFrame ();
}
void MagosuyaEngine::EndFrame () {
	imGuiManager_->Draw ();
	dxCommon_->EndFrame ();
	textureManager_->ClearIntermediateResource ();
}

void MagosuyaEngine::Finalize () {
	dxCommon_->Finalize ();
}

void MagosuyaEngine::LoadTexture (const std::string& filePath, const std::string& ID) {
	textureManager_->LoadTexture (filePath, ID);
}

D3D12_GPU_DESCRIPTOR_HANDLE* MagosuyaEngine::GetTextureHandle (const std::string& ID) {
	return textureManager_->GetTextureHandle (ID);
}