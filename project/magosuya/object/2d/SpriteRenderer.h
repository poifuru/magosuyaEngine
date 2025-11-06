#pragma once
#include <d3d12.h>
#include <Wrl.h>
using namespace Microsoft::WRL;
#include "../../../general/struct.h"

class DxCommon; //前方宣言

class SpriteRenderer {
public:		//外部公開メソッド
	SpriteRenderer (DxCommon* dxCommon);
	~SpriteRenderer ();

	void Initialize (Vector2 size);
	void Update (Matrix4x4 wvpData, Transform uvTransform);
	void Draw (D3D12_GPU_DESCRIPTOR_HANDLE textureHandle);
	void ImGui (Transform& transform, Transform& uvTransform);

private:	//メンバ変数
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
	VertexData* vertexData_ = nullptr;
	uint32_t* indexData_ = nullptr;
	Matrix4x4* matrixData_ = nullptr;
	Material* materialData_ = nullptr;

	//ImGuiで色をいじる
	float color_[4];

	//ポインタを借りる
	DxCommon* dxCommon_ = nullptr;
};

