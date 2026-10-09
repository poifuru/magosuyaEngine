#pragma once
#include "BaseObject3d.h"

namespace MyEngine::Rendering {
	struct ModelData;
	struct Node;
}

namespace MyEngine::Rendering {
	class Model : public BaseObject3d {
	public:
		Model();
		~Model() override = default;

		// アセットはファクトリーから直接流してもらう
		void Initialize(MyEngine::Rendering::ModelData* modelData, ID3D12Device* device = nullptr);

		MyEngine::Rendering::Material* GetMaterial();

		// モデルデータを外に渡せるように
		MyEngine::Rendering::ModelData* GetModelData() const { return modelData_; }
		void SetModelData(MyEngine::Rendering::ModelData* modelData) { modelData_ = modelData; }

		// シェーディングモデルの設定
		void SetShaders(MyEngine::Rendering::ShadingModel shadingModel);
		void SetTextureIndex(uint32_t textureIndex);

		// ノード検索・行列更新用ヘルパー関数
		Node* FindNode(const std::string& name);
		const Node* FindNode(const std::string& name) const;
		void UpdateNodeTransforms();

		// 各メッシュ用 Transform バッファの GPU アドレス取得
		D3D12_GPU_VIRTUAL_ADDRESS GetMeshTransformGPUAddress(uint32_t meshIndex) const;
		void Update(const CameraData* cameraData) override;

	private:
		MyEngine::Rendering::ModelData* modelData_ = nullptr;
		std::vector<std::unique_ptr<TransformMatrixResource>> meshTransformBuffers_;
	};
}