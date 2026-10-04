#include "PCH.h"
#include "SceneManager.h"
#include "RenderSystem.h"
#include "GraphicsDevice.h"
#include "GameTime.h"

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
	context_.sceneManager = this;
}

void SceneManager::Update(CameraData* cameraData) {
	// シーンの遷移中なら
	if(transitionState_ == SceneTransitionState::Transitioning) {
		// タイマーを進める
		transitionTimer_ += Time::GetDeltaTime();

		// 両方のシーンを更新
		if(currentScene_) {
			currentScene_->Update(cameraData);
		}
		if(nextScene_) {
			nextScene_->Update(cameraData);
		}

		// 予定の時間が立ったら遷移完了
		if(transitionTimer_ >= transitionDuration_) {
			CommandManager::GetInstance()->Clear();
			currentScene_ = std::move(nextScene_);
			nextScene_ = nullptr;
			transitionState_ = SceneTransitionState::None;
			transitionTimer_ = 0.0f;
			transitionDuration_ = 0.0f;
		}
	}
	else {
		// 遷移していない時は現在シーンだけを更新
		if(currentScene_) {
			currentScene_->Update(cameraData);
		}
	}
}

void SceneManager::Draw(MyEngine::Rendering::Renderer* renderer) {
	// 現在シーンを描画
	if (currentScene_) {
		currentScene_->Draw(renderer);
	}

	// 遷移中なら次のシーンも重ねて描画
	if(transitionState_ == SceneTransitionState::Transitioning && nextScene_) {
		nextScene_->Draw(renderer);
	}
}

void SceneManager::DrawUI() {
	if (currentScene_) {
		currentScene_->DrawUI();
	}
	if (transitionState_ == SceneTransitionState::Transitioning && nextScene_) {
		nextScene_->DrawUI();
	}
}

PostEffectManager* SceneManager::GetPostEffectManager() const {
	if (currentScene_) {
		return currentScene_->GetPostEffectManager();
	}
	return nullptr;
}

float SceneManager::GetTransitionPogress() const {
	if(transitionDuration_ <= 0.0f) {
		return 1.0f;
	}

	float progress = transitionTimer_ / transitionDuration_;
	if(progress > 1.0f) {
		progress = 1.0f;
	}

	return progress;
}
