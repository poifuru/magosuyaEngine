#pragma once
#include "BaseScene.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "ModelFactory.h"
#include "CommandManager.h"

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
	None,			// 通常(単一シーンでの稼働)
	Transitioning	// 遷移中(2つのシーンを平行して稼働)
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
	bool isTransitioning() const { return transitionState_ == SceneTransitionState::Transitioning; }
	float GetTransitionPogress() const;

	// シーン遷移用のテンプレート関数
	template <typename T>
	void ChangeScene(float duration = 0.0f) {
		// 初回またはdurationが0以下の場合は即時切り替え
		if(!currentScene_ || duration <= 0.0f) {
			CommandManager::GetInstance()->Clear();

			auto next = std::make_unique<T>();
			next->SetContext(&context_);
			next->SetRenderer(renderer_);
			next->Initialize();

			currentScene_ = std::move(next);
			nextScene_ = nullptr;

			transitionState_ = SceneTransitionState::None;
			transitionTimer_ = 0.0f;
			transitionDuration_ = 0.0f;

			return;
		}

		// すでに遷移中の場合は多重呼び出しを無視する
		if(transitionState_ == SceneTransitionState::Transitioning) {
			return;
		}

		// 次のシーンを生成して初期化
		auto next = std::make_unique<T>();
		next->SetContext(&context_);
		next->SetRenderer(renderer_);
		next->Initialize();
		nextScene_ = std::move(next);

		// 遷移タイマー開始
		transitionDuration_ = duration;
		transitionTimer_ = 0.0f;
		transitionState_ = SceneTransitionState::Transitioning;
	}

private:
	std::unique_ptr<BaseScene> currentScene_ = nullptr;		// 現在シーン
	std::unique_ptr<BaseScene> nextScene_ = nullptr;		// 遷移先のシーン

	// シーン遷移制御用の変数
	SceneTransitionState transitionState_ = SceneTransitionState::None;
	float transitionDuration_ = 0.0f;	// 遷移にかける秒数
	float transitionTimer_ = 0.0f;		// 経過時間タイマー

	// マネージャーの実体を SceneManager が所有する
	std::unique_ptr<TextureManager> textureManager_ = nullptr;
	std::unique_ptr<ModelManager> modelManager_ = nullptr;
	std::unique_ptr<ModelFactory> modelFactory_ = nullptr;

	// 各シーンへ配布する SceneContext の情報
	SceneContext context_;

	// RenderSystemのポインタを借りる
	MyEngine::Rendering::Renderer* renderer_ = nullptr;
};
