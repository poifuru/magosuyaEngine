#pragma once

class TextureManager;
class ModelFactory;
class ModelManager;
struct CameraData;
class GameObject;

namespace MyEngine::LowLevel {
	class GraphicsDevice;
	class DescriptorHeapManager;
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

// シーンで必要になる高レベルマネージャーや低レイヤー参照のポインタを束ねた薄い構造体
struct SceneContext {
	TextureManager* textureManager = nullptr;
	ModelFactory* modelFactory = nullptr;
	ModelManager* modelManager = nullptr;
	MyEngine::LowLevel::GraphicsDevice* graphicsDevice = nullptr;
	MyEngine::LowLevel::DescriptorHeapManager* heapManager = nullptr;
	MyEngine::Rendering::RootSignatureManager* rootSigManager = nullptr;
	MyEngine::Rendering::PSOManager* psoManager = nullptr;
	MyEngine::Rendering::ShaderManager* shaderManager = nullptr;
	MyEngine::Rendering::InputLayoutManager* inputLayoutManager = nullptr;
	MyEngine::Rendering::BlendModeManager* blendModeManager = nullptr;

	// 動的追加のためにオブジェクトリストのポインタを載せる
	std::vector<std::unique_ptr<GameObject>>* gameObjects = nullptr;

	// 現在シーンに存在する生存オブジェクトリストへのポインタ
	std::vector<std::unique_ptr<GameObject>>* activeGameObjects = nullptr;
};

class BaseScene {
public:
	virtual ~BaseScene() = default;

	// コンテキストの注入
	void SetContext(SceneContext* context) {
		context_ = context;
	}

	void SetRenderer(MyEngine::Rendering::Renderer* renderer) { renderer_ = renderer; }

	virtual void Initialize() = 0;
	virtual void Update(CameraData* cameraData) = 0;
	virtual void Draw(MyEngine::Rendering::Renderer* renderer) = 0;
	virtual void DrawUI() {}

	virtual PostEffectManager* GetPostEffectManager() { return nullptr; }

	SceneContext* GetContext() const { return context_; }

protected:
	// 借りてきたポインタ群
	SceneContext* context_ = nullptr;
	MyEngine::Rendering::Renderer* renderer_ = nullptr;
};
