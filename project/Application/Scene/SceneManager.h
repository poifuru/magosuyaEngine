#pragma once
#include "BaseScene.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "ModelFactory.h"
#include "CommandManager.h"
#include "PostEffectManager.h"
#include "Dissolve.h"
#include "EditorManager.h"

struct ID3D12GraphicsCommandList;
namespace MyEngine::LowLevel {
	class GraphicsDevice;
	class SrvDescriptorHeapPool;
}

namespace MyEngine::Rendering {
	class Renderer;
	class RootSignatureManager;
	class PSOManager;
	class ShaderManager;
	class InputLayoutManager;
	class BlendModeManager;
}
struct CameraData;

// シーンの遷移状態
enum class SceneTransitionState {
	None,	 // 通常(単一シーンでの稼働)
	FadeOut, // ディゾルブで溶けて真っ黒になっていく
	Loading, // 暗転状態で次シーンを同期ロード
	FadeIn,  // 次シーンがディゾルブで開いていく
};

class SceneManager {
public:
	SceneManager() = default;
	~SceneManager() = default;

	// 各マネージャーの初期化に必要なポインタ群を受け取って初期化
	void Initialize(
		MyEngine::LowLevel::GraphicsDevice* graphicsDevice,
		ID3D12GraphicsCommandList* cmdList,
		MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager,
		MyEngine::Rendering::RootSignatureManager* rootSigManager, 
		MyEngine::Rendering::PSOManager* psoManager,
		MyEngine::Rendering::ShaderManager* shaderManager,
		MyEngine::Rendering::InputLayoutManager* inputLayoutManager,
		MyEngine::Rendering::BlendModeManager* blendModeManager
	);

	void Update(CameraData* cameraData);
	void Draw(MyEngine::Rendering::Renderer* renderSystem);
	void DrawUI();

	MyEngine::Rendering::Renderer* GetRenderer() { return renderer_; }
	void SetRenderer(MyEngine::Rendering::Renderer* renderer) { renderer_ = renderer; }

	PostEffectManager* GetPostEffectManager() const;

	TextureManager* GetTextureManager() { return textureManager_.get(); }

	// 遷移中かどうか(進行度0.0 ~ 1.0)の取得
	bool isTransitioning() const { return transitionState_ != SceneTransitionState::None;  }
	float GetTransitionPogress() const;

	// シーン遷移用のテンプレート関数
	template <typename T>
	void ChangeScene() {
		// 遷移中なら受け付けない
		if (isTransitioning()) return;

		CommandManager::GetInstance()->Clear();
#ifdef USEIMGUI
		EditorManager::GetInstance()->ClearSelectedObject();
#endif
		auto next = std::make_unique<T>();
		next->SetContext(&context_);
		next->SetRenderer(renderer_);
		next->Initialize();
		currentScene_ = std::move(next);
		transitionState_ = SceneTransitionState::None;
#ifdef USEIMGUI
		EditorManager::GetInstance()->SetSceneContext(&context_);
#endif
	}

	// ディゾルブを使ったシーン遷移
	template <typename T>
	void ChangeSceneWithDissolve(float fadeOutDuration = 0.8f, float fadeInDuration = 0.8f) {
		if (transitionState_ != SceneTransitionState::None) return;

		fadeOutDuration_ = fadeOutDuration;
		fadeInDuration_ = fadeInDuration;
		fadeTimer_ = 0.0f;
		transitionState_ = SceneTransitionState::FadeOut;

		// 次シーン生成用ヘルパーのポインタを保持
		pendingSceneCreator_ = &SceneManager::CreateSceneHelper<T>;

		// 現在のシーンのディゾルブをONにして初期化
		if (auto* pe = GetPostEffectManager()) {
			pe->SetEffectActive(PostEffectType::Dissolve, true);
			if (auto* dissolve = pe->GetEffect<Dissolve>(PostEffectType::Dissolve)) {
				dissolve->SetThreshold(0.0f);
			}
		}
	}

private:
	template <typename T>
	static std::unique_ptr<BaseScene> CreateSceneHelper() {
		return std::make_unique<T>();
	}

private:
	std::unique_ptr<BaseScene> currentScene_ = nullptr;		// 現在シーン

	typedef std::unique_ptr<BaseScene>(*SceneCreatorFunc)();
	SceneCreatorFunc pendingSceneCreator_ = nullptr;

	// シーン遷移制御用の変数
	SceneTransitionState transitionState_ = SceneTransitionState::None;
	float fadeOutDuration_ = 0.8f;
	float fadeInDuration_ = 0.8f;
	float fadeTimer_ = 0.0f;

	// マネージャーの実体を SceneManager が所有する
	std::unique_ptr<TextureManager> textureManager_ = nullptr;
	std::unique_ptr<ModelManager> modelManager_ = nullptr;
	std::unique_ptr<ModelFactory> modelFactory_ = nullptr;

	// 各シーンへ配布する SceneContext の情報
	SceneContext context_;

	// RenderSystemのポインタを借りる
	MyEngine::Rendering::Renderer* renderer_ = nullptr;
};
