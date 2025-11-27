#include "MagosuyaEngine.h"
#include "Windows.h"
#include "DxCommon.h"
#include "TextureManager.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "PSOManager.h"
#include "LineRenderer.h"

MagosuyaEngine::~MagosuyaEngine () {
	dxCommon_->Finalize ();
}

void MagosuyaEngine::Initialize () {
	winApi_ = WindowsAPI::GetInstance ();
	winApi_->Initialize (InputManager::GetInstance ());
	dxCommon_ = DxCommon::GetInstance ();
	dxCommon_->Initialize ();
	imguiManager_ = ImGuiManager::GetInstance ();
	imguiManager_->Initialize ();
	inputManager_ = InputManager::GetInstance ();
	inputManager_->Initialize (winApi_->GetHwnd ());
	texManager_ = TextureManager::GetInstance ();
	texManager_->Initialize (dxCommon_);
	modelManager_ = ModelManager::GetInstance ();
	modelManager_->Initialize (dxCommon_, texManager_);
	psoManager_ = PSOManager::GetInstance ();
	psoManager_->Initialize (dxCommon_);
	lineRenderer_ = LineRenderer::GetInstance ();
	lineRenderer_->Initialize (dxCommon_);
}

void MagosuyaEngine::BeginFrame () {
	imguiManager_->BeginFrame ();
	dxCommon_->BeginFrame ();
}
void MagosuyaEngine::EndFrame () {
	imguiManager_->Draw ();
	inputManager_->EndFrame ();
	dxCommon_->EndFrame ();
	texManager_->ClearIntermediateResource ();
}