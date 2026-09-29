#include "PCH.h"
#include "SceneManager.h"
#include "RenderSystem.h"
#include "GraphicsDevice.h"

void SceneManager::Initialize(
	MyEngine::LowLevel::GraphicsDevice* graphicsDevice,
	ID3D12GraphicsCommandList* cmdList,
	MyEngine::LowLevel::SrvDescriptorHeap* heapManager,
	MyEngine::Rendering::RootSignatureManager* rootSigManager, 
	MyEngine::Rendering::PSOManager* psoManager,
	MyEngine::Rendering::ShaderManager* shaderManager,
	MyEngine::Rendering::InputLayoutManager* inputLayoutManager,
	MyEngine::Rendering::BlendModeManager* blendModeManager
) {
	// マネージャー群を初期化
	textureManager_.Initialize(graphicsDevice->GetDevice(), cmdList, heapManager);
	modelManager_.Initialize(graphicsDevice->GetDevice(), &textureManager_);
	modelFactory_.Initialize(graphicsDevice,
							 heapManager,
							 &modelManager_,
							 &textureManager_
	);

	// シーン配布用のコンテキストを組み立てる
	context_.graphicsDevice = graphicsDevice;
	context_.heapManager = heapManager;
	context_.textureManager = &textureManager_;
	context_.modelFactory = &modelFactory_;
	context_.modelManager = &modelManager_;
	context_.rootSigManager = rootSigManager;
	context_.psoManager = psoManager;
	context_.shaderManager = shaderManager;
	context_.inputLayoutManager = inputLayoutManager;
	context_.blendModeManager = blendModeManager;
}

void SceneManager::Update(CameraData* cameraData) {
	if (currentScene_) {
		currentScene_->Update(cameraData);
	}
}

void SceneManager::Draw(MyEngine::Rendering::Renderer* renderer) {
	if (currentScene_) {
		currentScene_->Draw(renderer);
	}
}

void SceneManager::DrawUI() {
	if (currentScene_) {
		currentScene_->DrawUI();
	}
}