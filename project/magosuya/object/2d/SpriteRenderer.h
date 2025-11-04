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

	void Initialize (TransformData* transformData, Vector2 size);
	void Update ();
	void Draw (D3D12_GPU_DESCRIPTOR_HANDLE textureHandle);
	void ImGui ();

private:	//メンバ変数
	//TransformDataのポインタ
	TransformData* transformData_ = nullptr;

	//GPUリソース
	ComPtr<ID3D12Resource> vertexBuffer_;
	ComPtr<ID3D12Resource> indexBuffer_;
	ComPtr<ID3D12Resource> matrixBuffer_;
	ComPtr<ID3D12Resource> materialBuffer_;

	D3D12_VERTEX_BUFFER_VIEW vbView_{};
	D3D12_INDEX_BUFFER_VIEW ibView_{};

	//ポインタを借りる
	DxCommon* dxCommon_ = nullptr;
};

