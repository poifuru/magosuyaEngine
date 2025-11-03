#include "MagosuyaEngine.h"

MagosuyaEngine::MagosuyaEngine () {
	dxCommon_ = std::make_unique<DxCommon> ();
	textureManager_ = std::make_unique<TextureManager> (dxCommon_.get ());
}

MagosuyaEngine::~MagosuyaEngine () {

}

void MagosuyaEngine::Initialize () {
	dxCommon_->Initialize ();
	textureManager_->Initialize ();
}

void MagosuyaEngine::BeginFrame () {
	dxCommon_->BeginFrame ();
}
void MagosuyaEngine::EndFrame () {
	dxCommon_->EndFrame ();
	textureManager_->ClearIntermediateResource ();
}

void MagosuyaEngine::Finalize () {
	dxCommon_->Finalize ();
}