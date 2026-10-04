#include "PCH.h"
#include "SceneManager.h"
#include "RenderSystem.h"
#include "GraphicsDevice.h"

void SceneManager::Initialize(
	MyEngine::LowLevel::GraphicsDevice* graphicsDevice,
	ID3D12GraphicsCommandList* cmdList,
	MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager,
	MyEngine::Rendering::RootSignatureManager* rootSigManager, 
	MyEngine::Rendering::PSOManager* psoManager,
	MyEngine::Rendering::ShaderManager* shaderManager,
	MyEngine::Rendering::InputLayoutManager* inputLayoutManager,
	MyEngine::Rendering::BlendModeManager* blendModeManager
) {
	// マネージャー群を初期化
	textureManager_ = std::make_unique<TextureManager>();
	textureManager_->Initialize(graphicsDevice->GetDevice(), cmdList, heapManager);

	modelManager_ = std::make_unique<ModelManager>();
	modelManager_->Initialize(graphicsDevice->GetDevice(), textureManager_.get());

	modelFactory_ = std::make_unique<ModelFactory>();
	modelFactory_->Initialize(graphicsDevice,
							 heapManager,
							 modelManager_.get(),
							 textureManager_.get()
	);

	// シーン配布用のコンテキストを組み立てる
	context_.graphicsDevice = graphicsDevice;
	context_.heapManager = heapManager;
	context_.textureManager = textureManager_.get();
	context_.modelFactory = modelFactory_.get();
	context_.modelManager = modelManager_.get();
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

PostEffectManager* SceneManager::GetPostEffectManager() const {
	if (currentScene_) {
		return currentScene_->GetPostEffectManager();
	}
	return nullptr;
}