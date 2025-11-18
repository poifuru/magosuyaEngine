#pragma once
#include <d3d12.h>
#include <Wrl.h>
using namespace Microsoft::WRL;
#include <vector>
#include <memory>
#include "struct.h"

class MagosuyaEngine;

class Particle {
public:
	Particle (MagosuyaEngine* magosuya);
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

	//ルートシグネチャとパイプラインステート
	ComPtr<ID3D12RootSignature> rootSignature_;
	ComPtr<ID3D12PipelineState> pipelineState_;

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
	MagosuyaEngine* magosuya_ = nullptr;
};

