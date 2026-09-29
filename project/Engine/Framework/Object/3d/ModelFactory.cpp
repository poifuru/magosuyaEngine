#include "PCH.h"
#include "ModelFactory.h"
#include "Model.h"
#include "MeshData.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "GraphicsDevice.h"
#include "SrvDescriptorHeapPool.h"

void ModelFactory::Initialize(
	MyEngine::LowLevel::GraphicsDevice* device,
	MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager,
	ModelManager* modelManager,
	TextureManager* textureManager
) {
	device_ = device;
	heapManager_ = heapManager;
	modelManager_ = modelManager;
	textureManager_ = textureManager;
}

std::unique_ptr<MyEngine::Rendering::Model> ModelFactory::CreateModel(
	uint32_t modelIndex,
	uint32_t textureIndex
) {
	// 各マネージャーからアセットパーツを取得
	auto modelData = modelManager_->GetModelData(modelIndex);
	auto tempModelData = modelData.lock().get();

	// Modelインスタンスを生成（この時点では空）
	auto model = std::make_unique<MyEngine::Rendering::Model>();

	// 親クラス用のバッファを生成・初期化
	// Transformバッファをモデルへ設定
	auto transformBuffer = std::make_unique<TransformMatrixResource>();
	transformBuffer->Initialize(device_->GetDevice());
	model->SetTransformBuffer(std::move(transformBuffer));

	// textureIndexが0のとき、モデル側にデフォルトテクスチャがあればそれを使う
	uint32_t finalTextureIndex = textureIndex;
	if (finalTextureIndex == 0 && !tempModelData->defaultTextureIndices.empty()) {
		finalTextureIndex = tempModelData->defaultTextureIndices[0];
	}

	// 新しいマテリアルを作成して初期化
	auto material = std::make_shared<MyEngine::Rendering::Material>();
	material->Initialize(device_, heapManager_);

	material->SetTextureIndex(finalTextureIndex);
	material->SetShadingModel(MyEngine::Rendering::ShadingModel::Standard);
	// モデルにマテリアルをセット
	model->SetMaterial(material);

	// アセットの設定
	model->Initialize(tempModelData, device_->GetDevice());

	return model;
}
