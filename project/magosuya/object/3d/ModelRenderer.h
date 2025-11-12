#pragma once
#include <d3d12.h>
#include <Wrl.h>
using namespace Microsoft::WRL;
#include <vector>
#include "struct.h"

class MagosuyaEngine;

class ModelRenderer {
public:
	ModelRenderer (MagosuyaEngine* magosuya);
	~ModelRenderer ();

	void Initialize ();
	void Update (Matrix4x4 world, Transform wvp);
	void Draw (D3D12_GPU_DESCRIPTOR_HANDLE textureHandle);

	//アクセッサ
	Material* GetMaterial () { return materialData_; }
	void SetColor (const Vector4& color) { materialData_->color = color; }
	void SetModelID (const std::string& id) { modelID_ = id; }
	void SetTextureID (const std::string& id) { texID_ = id; }

private:
	//ルートシグネチャとパイプラインステート
	ComPtr<ID3D12RootSignature> rootSignature_;
	ComPtr<ID3D12PipelineState> pipelineState_;

	//GPUリソース
	ComPtr<ID3D12Resource> vertexBuffer_;
	ComPtr<ID3D12Resource> indexBuffer_;
	ComPtr<ID3D12Resource> matrixBuffer_;
	ComPtr<ID3D12Resource> materialBuffer_;

	D3D12_VERTEX_BUFFER_VIEW vbView_{};
	D3D12_INDEX_BUFFER_VIEW ibView_{};

	//GPUリソースにマッピングするデータ
	std::vector<VertexData> vertexData_;
	VertexData* vertexDataPtr_ = nullptr;
	uint32_t* indexData_ = nullptr;
	TransformationMatrix* matrixData_ = nullptr;
	Material* materialData_ = nullptr;

	//モデルデータ
	ModelData modelData_;

	TransformationMatrix* matrixData_ = nullptr;

	//モデルデータ検索用ID
	std::string modelID_;
	//テクスチャ検索用ID
	std::string texID_;

	//ImGuiで色をいじる
	float color_[4];

	//ポインタを借りる
	MagosuyaEngine* magosuya_ = nullptr;
};