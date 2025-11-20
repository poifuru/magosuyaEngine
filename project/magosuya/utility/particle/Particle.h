#pragma once
#include <Windows.h>
#include <Wrl.h>
using namespace Microsoft::WRL;
#include <d3d12.h>
#include <vector>
#include <memory>
#include "struct.h"

class DxCommon;

class Particle {
public:
	Particle (DxCommon* dxCommon);
	~Particle ();

	void Initialize ();
	void Update (Matrix4x4* vp);
	void Draw ();

	void SetTexHandle (D3D12_GPU_DESCRIPTOR_HANDLE handle) { handle_ = handle; }
	Transform GetTransform () { return transform_; }
	void SetTransform (Transform transform) { transform_ = transform; }
	Transform GetUVTransform () { return uvTransform_; }
	void SetUVTransform (Transform uvTransform) { uvTransform_ = uvTransform; }

private:
	//モデルデータ
	std::unique_ptr<ModelData> data_ = nullptr;

	//GPUリソース
	ComPtr<ID3D12Resource> matrixBuffer_;
	ComPtr<ID3D12Resource> materialBuffer_;

	//マッピング用のCPUデータ
	VertexData* vertexData_ = nullptr;
	uint32_t* indexData_ = nullptr;
	TransformationMatrix* matrixData_ = nullptr;
	Material* materialData_ = nullptr;

	//テクスチャハンドル
	D3D12_GPU_DESCRIPTOR_HANDLE handle_;

	//位置データ
	Transform transform_;
	Transform uvTransform_;

	//ポインタを借りる
	DxCommon* dxCommon_ = nullptr;
	ID3D12GraphicsCommandList* commandList_ = nullptr;
};

