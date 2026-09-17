#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Animation.h"

class TextureManager;
struct ID3D12Device;

namespace MyEngine::Rendering {
	struct ModelData;
	struct Node;
	struct MaterialTex;
}

class ModelManager {
public:
	ModelManager() = default;
	~ModelManager() = default;

	void Initialize(ID3D12Device* device, TextureManager* textureManager);

	uint32_t LoadModelData(const std::string& filePath, bool inversion = false);
	std::weak_ptr<MyEngine::Rendering::ModelData> GetModelData(uint32_t index);
	void UnloadModelData(const std::string& filePath, bool inversion = false);

	Animation* LoadAnimationData(const std::string& directoryPath, const std::string& fileName);
	std::weak_ptr<Animation> GetAnimationData(std::string id);
	void UnloadAnimationData(const std::string& id);

public:
	// コピー・移動禁止
	ModelManager(const ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;
	ModelManager(ModelManager&&) = delete;
	ModelManager& operator=(ModelManager&&) = delete;

private:
	// マテリアルファイルの読み込み関数
	MyEngine::Rendering::MaterialTex LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& id);

	// モデル読み込みの関数
	MyEngine::Rendering::ModelData LoadModelFile(const std::string& filePath, bool inversion);

	// assimpのノードからNode構造体に変換する関数
	MyEngine::Rendering::Node ReadNode(aiNode* node);

	// アニメーション読み込み関数
	Animation LoadAnimation(const std::string& directoryPath, const std::string& fileName);

	// DX12バッファ生成用の内部ヘルパー
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);

	// マップキー作成関数
	std::string MakeKey(const std::string& filePath, bool inversion);

private:
	std::vector<std::shared_ptr<MyEngine::Rendering::ModelData>> models_;
	std::unordered_map<std::string, uint32_t> modelPathToIndexMap_;
	std::unordered_map<std::string, std::shared_ptr<Animation>> animationMap_;

	ID3D12Device* device_ = nullptr;
	TextureManager* textureManager_ = nullptr;
};