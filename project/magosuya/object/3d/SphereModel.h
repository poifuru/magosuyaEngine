#pragma once
#include <Windows.h>
#include <Wrl.h>
using namespace Microsoft::WRL;
#include <d3d12.h>
#include <vector>
#include "DxCommon.h"
#include "struct.h"

class SphereModel {
public:		//メンバ関数
	SphereModel (DxCommon* dxCommon, int subdivision);
	~SphereModel ();

	void Initialize (Vector3 position, float radius);
	void Update (Matrix4x4* view, Matrix4x4* proj);
	void Draw (D3D12_GPU_DESCRIPTOR_HANDLE textureHandle);

	void ShowImGuiEditor ();

	//アクセッサ
	std::vector<VertexData> GetVertexData () { return vertexData_; }
	Matrix4x4 GetWorld () { return transformationMatrix_.World; }

private:	//メンバ変数
	ComPtr<ID3D12Resource> vertexBuffer_;
	//ComPtr<ID3D12Resource> indexBuffer_;
	ComPtr<ID3D12Resource> matrixBuffer_;
	ComPtr<ID3D12Resource> materialBuffer_;

	std::vector<VertexData> vertexData_;
	VertexData* vertexDataPtr_;    // GPU側への書き込みポインタ
	//std::vector<uint32_t> indexData_;
	TransformationMatrix* matrixData_ = nullptr;
	Material* materialData_ = nullptr;

	//バッファビュー
	D3D12_VERTEX_BUFFER_VIEW vbView_{};
	D3D12_INDEX_BUFFER_VIEW ibView_{};

	//座標データ
	Transform transform_;	//ローカル座標
	Transform uvTransform_;	//uvTranform用のローカル座標
	TransformationMatrix transformationMatrix_; //ワールド座標と、カメラからwvp行列をもらって格納する

	//球の半径
	float radius_;
	
	//球の縦横の分割数
	int kSubdivision_;

	//ImGuiで色を変える
	float color_[4];

	//dxCommonのポインタを持たせる
	DxCommon* dxCommon_ = nullptr;
};

