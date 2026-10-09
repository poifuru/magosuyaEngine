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

void SceneManager::Update(const CameraData* cameraData) {
	float dt = Time::GetDeltaTime();

	switch (transitionState_) {
	case SceneTransitionState::None:
		if (currentScene_) {
			currentScene_->Update(cameraData);
		}

		break;

	case SceneTransitionState::FadeOut:
		// 現在のシーンを動かしながら、ディゾルブで溶かしていく
		if (currentScene_) {
			currentScene_->Update(cameraData);
		}

		fadeTimer_ += dt;

		{
			float rate = fadeTimer_ / fadeOutDuration_;
			if (rate > 1.0f) rate = 1.0f;
			if (auto* pe = GetPostEffectManager()) {
				if (auto* dissolve = pe->GetEffect<Dissolve>(PostEffectType::Dissolve)) {
					dissolve->SetThreshold(rate);
				}
			}
			// 完全に溶け切って真っ黒になった
			if (rate >= 1.0f) {
				transitionState_ = SceneTransitionState::Loading;
			}
		}

		break;

	case SceneTransitionState::Loading:
		// 画面が完全に真っ黒の状態で安全に同期ロード
		if (pendingSceneCreator_) {
			CommandManager::GetInstance()->Clear();
#ifdef USEIMGUI
			EditorManager::GetInstance()->ClearSelectedObject();
#endif

			auto next = pendingSceneCreator_();
			next->SetContext(&context_);
			next->SetRenderer(renderer_);
			next->Initialize();
			currentScene_ = std::move(next);
			pendingSceneCreator_ = nullptr;
#ifdef USEIMGUI
			EditorManager::GetInstance()->SetSceneContext(&context_);
#endif

			// 新しいシーンのディゾルブをONにして、真っ黒（threshold = 1.0f）から開始
			if (auto* pe = GetPostEffectManager()) {
				pe->SetEffectActive(PostEffectType::Dissolve, true);

				if (auto* dissolve = pe->GetEffect<Dissolve>(PostEffectType::Dissolve)) {
					dissolve->SetThreshold(1.0f);
				}
			}
		}

		fadeTimer_ = 0.0f;
		transitionState_ = SceneTransitionState::FadeIn;

		break;

	case SceneTransitionState::FadeIn:
		// 新しいシーンを動かしながら、ディゾルブを開いていく
		if (currentScene_) {
			currentScene_->Update(cameraData);
		}

		fadeTimer_ += dt;

		{
			float rate = 1.0f - (fadeTimer_ / fadeInDuration_);
			if (rate < 0.0f) rate = 0.0f;

			if (auto* pe = GetPostEffectManager()) {
				if (auto* dissolve = pe->GetEffect<Dissolve>(PostEffectType::Dissolve)) {
					dissolve->SetThreshold(rate);
				}
			}

			// 完全に開ききったら遷移完了
			if (rate <= 0.0f) {
				if (auto* pe = GetPostEffectManager()) {
					pe->SetEffectActive(PostEffectType::Dissolve, false); // エフェクトOFF
				}

				transitionState_ = SceneTransitionState::None;
			}
		}

		break;
	}
}

void SceneManager::Draw(MyEngine::Rendering::Renderer* renderer) {
	// 現在シーンを描画
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

float SceneManager::GetTransitionPogress() const {
	switch (transitionState_) {
	case SceneTransitionState::FadeOut:

		return fadeOutDuration_ > 0.0f ? (fadeTimer_ / fadeOutDuration_) : 1.0f;

	case SceneTransitionState::Loading:

		return 1.0f;

	case SceneTransitionState::FadeIn:

		return fadeInDuration_ > 0.0f ? (1.0f - (fadeTimer_ / fadeInDuration_)) : 0.0f;

	case SceneTransitionState::None:

	default:

		return 0.0f;
	}
}
