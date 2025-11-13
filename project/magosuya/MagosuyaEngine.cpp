#include "MagosuyaEngine.h"

MagosuyaEngine::MagosuyaEngine () {
	dxCommon_ = std::make_unique<DxCommon> ();
	imGuiManager_ = std::make_unique<ImGuiManager> (dxCommon_.get ());
	textureManager_ = std::make_unique<TextureManager> (dxCommon_.get ());
	modelManager_ = std::make_unique<ModelManager> (dxCommon_.get (), textureManager_.get());
}

MagosuyaEngine::~MagosuyaEngine () {

}

void MagosuyaEngine::Initialize () {
	dxCommon_->Initialize ();
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

void MagosuyaEngine::UnloadTexture (const std::string& filePath) {
	textureManager_->UnloadTexture (filePath);
}

D3D12_GPU_DESCRIPTOR_HANDLE MagosuyaEngine::GetTextureHandle (const std::string& ID) {
	return textureManager_->GetTextureHandle (ID);
}

const DirectX::TexMetadata& MagosuyaEngine::GetMetaData (const std::string& id) {
	return textureManager_->GetMetaData (id);
}

void MagosuyaEngine::LoadModelData (const std::string& filePath, const std::string& ID, bool inversion) {
	modelManager_->LoadModelData (filePath, ID, inversion);
}

void MagosuyaEngine::UnloadModelData (const std::string& ID) {
	modelManager_->UnloadModelData (ID);
}

std::weak_ptr<ModelData> MagosuyaEngine::GetModelData (const std::string& ID) {
	return modelManager_->GetModelData (ID);
}