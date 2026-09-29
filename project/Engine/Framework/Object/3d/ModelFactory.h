#pragma once

// 前方宣言
namespace MyEngine::LowLevel {
	class GraphicsDevice;
	class SrvDescriptorHeapPool;
}

namespace MyEngine::Rendering {
	class Model;
}

class ModelManager;
class TextureManager;

class ModelFactory {
public:
	ModelFactory() = default;
	~ModelFactory() = default;

	void Initialize(
		MyEngine::LowLevel::GraphicsDevice* device,
		MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager,
		ModelManager* modelManager,
		TextureManager* textureManager
	);

	// アセットが紐づいたModelを組み立てて出力する
	std::unique_ptr<MyEngine::Rendering::Model> CreateModel(
		uint32_t modelIndex,
		uint32_t textureIndex = 0
	);

public:
	// コピー・移動禁止
	ModelFactory(const ModelFactory&) = delete;
	ModelFactory& operator=(const ModelFactory&) = delete;
	ModelFactory(ModelFactory&&) = delete;
	ModelFactory& operator=(ModelFactory&&) = delete;

private:
	MyEngine::LowLevel::GraphicsDevice* device_ = nullptr;
	MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager_ = nullptr;

	ModelManager* modelManager_ = nullptr;
	TextureManager* textureManager_ = nullptr;
};
