#include "MagosuyaEngine.h"

MagosuyaEngine::MagosuyaEngine () {
	dxCommon_ = std::make_unique<DxCommon> ();
	imGuiManager_ = std::make_unique<ImGuiManager> (dxCommon_.get ());
	textureManager_ = std::make_unique<TextureManager> (dxCommon_.get ());
	modelManager_ = std::make_unique<ModelManager> (dxCommon_.get (), textureManager_.get ());
	inputManager_ = std::make_unique<InputManager> ();
	soundManager_ = std::make_unique<SoundManager> ();
}

MagosuyaEngine::~MagosuyaEngine () {

}

void MagosuyaEngine::Initialize () {
	dxCommon_->Initialize (inputManager_.get ());
	imGuiManager_->Initialize ();
	inputManager_->Initialize (dxCommon_->GetWinAPI ()->GetHwnd ());
}

void MagosuyaEngine::BeginFrame () {
	imGuiManager_->BeginFrame ();
	dxCommon_->BeginFrame ();

	//ゲームパッドの更新
	inputManager_->GetGamePad ()->Update ();
}
void MagosuyaEngine::EndFrame () {
	imGuiManager_->Draw ();
	dxCommon_->EndFrame ();
	textureManager_->ClearIntermediateResource ();
	inputManager_->EndFrame ();
}

void MagosuyaEngine::Finalize () {
	dxCommon_->Finalize ();
	soundManager_->Finalize ();
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

RawInput* MagosuyaEngine::GetRawInput () {
	return inputManager_->GetRawInput ();
}

GamePad* MagosuyaEngine::GetGamePad () {
	return inputManager_->GetGamePad ();
}

SoundData* MagosuyaEngine::LoadSound (const std::string& filePath, const std::string& id) {
	return soundManager_->LoadSound (filePath, id);
}

void MagosuyaEngine::UnloadSound (const std::string& id) {
	soundManager_->UnloadSound (id);
}

const SoundData& MagosuyaEngine::GetSound (const std::string& id) const{
	return soundManager_->GetSound (id);
}

IXAudio2* MagosuyaEngine::GetXAudio () {
	return soundManager_->GetXAudio ();
}

IXAudio2MasteringVoice* MagosuyaEngine::GetMasterVoice () {
	return soundManager_->GetMasterVoice ();
}
