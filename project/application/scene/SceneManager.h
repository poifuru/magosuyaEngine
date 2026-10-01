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

	PostEffectManager* GetPostEffectManager() const {
		if (currentScene_) {
			return currentScene_->GetPostEffectManager();
		}
		return nullptr;
	}

	TextureManager* GetTextureManager() { return textureManager_.get(); }

	// シーン遷移用のテンプレート関数
	template <typename T>
	void ChangeScene() {
		CommandManager::GetInstance()->Clear();
		auto nextScene = std::make_unique<T>();
		nextScene->SetContext(&context_);
		nextScene->SetRenderer(renderer_);
		nextScene->Initialize();
		currentScene_ = std::move(nextScene);
	}

private:
	std::unique_ptr<BaseScene> currentScene_ = nullptr;

	// マネージャーの実体を SceneManager が所有する
	std::unique_ptr<TextureManager> textureManager_ = nullptr;
	std::unique_ptr<ModelManager> modelManager_ = nullptr;
	std::unique_ptr<ModelFactory> modelFactory_ = nullptr;

	// 各シーンへ配布する SceneContext の情報
	SceneContext context_;

	// RenderSystemのポインタを借りる
	MyEngine::Rendering::Renderer* renderer_ = nullptr;
};
