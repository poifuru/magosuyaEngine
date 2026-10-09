#pragma once

class TextureManager;
class ModelFactory;
class ModelManager;
struct CameraData;
class GameObject;

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

class PostEffectManager;
class SceneManager;

// シーンで必要になる高レベルマネージャーや低レイヤー参照のポインタを束ねた薄い構造体
struct SceneContext {
	TextureManager* textureManager = nullptr;
	ModelFactory* modelFactory = nullptr;
	ModelManager* modelManager = nullptr;
	MyEngine::LowLevel::GraphicsDevice* graphicsDevice = nullptr;
	MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager = nullptr;
	MyEngine::Rendering::RootSignatureManager* rootSigManager = nullptr;
	MyEngine::Rendering::PSOManager* psoManager = nullptr;
	MyEngine::Rendering::ShaderManager* shaderManager = nullptr;
	MyEngine::Rendering::InputLayoutManager* inputLayoutManager = nullptr;
	MyEngine::Rendering::BlendModeManager* blendModeManager = nullptr;

	// 動的追加のためにオブジェクトリストのポインタを載せる
	std::vector<std::unique_ptr<GameObject>>* gameObjects = nullptr;

	// 現在シーンに存在する生存オブジェクトリストへのポインタ
	std::vector<std::unique_ptr<GameObject>>* activeGameObjects = nullptr;

	// シーン遷移の進行状況を知るため
	SceneManager* sceneManager = nullptr;
};

class BaseScene {
public:
	// デストラクタ
	virtual ~BaseScene() = default;

	// コンテキストの注入
	void SetContext(SceneContext* context) {
		context_ = context;
	}

	// Rendererをセット
	void SetRenderer(MyEngine::Rendering::Renderer* renderer) { renderer_ = renderer; }

	// 初期化
	virtual void Initialize() = 0;

	// 更新
	void Update(const CameraData* cameraData);

	// 描画
	virtual void Draw(MyEngine::Rendering::Renderer* renderer) = 0;

	// UI描画
	virtual void DrawUI() {}

	// ポストエフェクトマネージャを取得
	virtual PostEffectManager* GetPostEffectManager() { return nullptr; }

	// Contextを取得
	SceneContext* GetContext() const { return context_; }

protected:
	// 子クラスはこのUpdateを継承
	virtual void UpdateGame(const CameraData* cameraData) = 0;
	virtual void UpdateEdit(const CameraData* cameraData);

protected:
	// 借りてきたポインタ群
	SceneContext* context_ = nullptr;
	MyEngine::Rendering::Renderer* renderer_ = nullptr;
};
