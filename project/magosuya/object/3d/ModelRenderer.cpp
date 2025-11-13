#include "ModelRenderer.h"
#include "mathFunction.h"
#include "MagosuyaEngine.h"

ModelRenderer::ModelRenderer (MagosuyaEngine* magosuya) {
	magosuya_ = magosuya;
	rootSignature_ = magosuya_->GetDxCommon ()->GetRootSignature ();
	pipelineState_ = magosuya_->GetDxCommon ()->GetPipelineState ();
	color_ = {};
}

ModelRenderer::~ModelRenderer () {
}

void ModelRenderer::Initialize () {
	//===リソースの初期化===//
	//頂点データ
	vertexData_.resize (modelData_.vertexCount);
	vertexBuffer_ = magosuya_->GetDxCommon()->CreateBufferResource (sizeof (VertexData) * modelData_.vertexCount);
	vertexBuffer_->Map (0, nullptr, reinterpret_cast<void**>(&vertexDataPtr_));
	vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress ();
	vbView_.SizeInBytes = UINT (sizeof (VertexData) * vertexData_.size ());
	vbView_.StrideInBytes = sizeof (VertexData);

	//お前は今じゃない、また今度な！
	//indexBuffer_ = CreateBufferResource (device, sizeof (uint32_t) * (kSubdivision_ * kSubdivision_) * 6);
	//indexBuffer_->Map (0, nullptr, reinterpret_cast<void**>(&indexData_));
	//ibView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress ();
	//ibView_.SizeInBytes = UINT (sizeof (uint32_t) * (kSubdivision_ * kSubdivision_) * 6);
	//ibView_.Format = DXGI_FORMAT_R32_UINT;

	//行列データ
	matrixBuffer_ = magosuya_->GetDxCommon ()->CreateBufferResource (sizeof (TransformationMatrix));
	matrixBuffer_->Map (0, nullptr, reinterpret_cast<void**>(&matrixData_));
	matrixData_->World = MakeIdentity4x4 ();
	matrixData_->WVP = MakeIdentity4x4 ();
	matrixData_->WorldInverseTranspose = MakeIdentity4x4 ();

	//マテリアルデータ
	materialBuffer_ = magosuya_->GetDxCommon ()->CreateBufferResource (sizeof (Material));
	materialBuffer_->Map (0, nullptr, reinterpret_cast<void**>(&materialData_));
	materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData_->enableLighting = true;
	materialData_->uvTranform = MakeIdentity4x4 ();

	color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
}

void ModelRenderer::Update (Matrix4x4 world, Matrix4x4 vp, Transform uvTransform) {
	matrixData_->World = world;
	matrixData_->WVP = Multiply(matrixData_->World, vp);
	matrixData_->WorldInverseTranspose = Transpose (Inverse (matrixData_->World));

	//uvTranform更新
	materialData_->uvTranform = MakeAffineMatrix (uvTransform.scale, uvTransform.rotate, uvTransform.translate);
}

void ModelRenderer::Draw (D3D12_GPU_DESCRIPTOR_HANDLE textureHandle) {
	//どんな形状で描画するのか
	magosuya_->GetDxCommon ()->GetCommandList ()->IASetPrimitiveTopology (D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//頂点バッファをセットする
	magosuya_->GetDxCommon ()->GetCommandList ()->IASetVertexBuffers (0, 1, &vbView_);	//VBVを設定
	//定数バッファのルートパラメータを設定する	
	magosuya_->GetDxCommon ()->GetCommandList ()->SetGraphicsRootConstantBufferView (0, matrixBuffer_->GetGPUVirtualAddress ());
	magosuya_->GetDxCommon ()->GetCommandList ()->SetGraphicsRootConstantBufferView (1, materialBuffer_->GetGPUVirtualAddress ());
	//テクスチャのSRVを設定
	magosuya_->GetDxCommon ()->GetCommandList ()->SetGraphicsRootDescriptorTable (2, textureHandle);
	//実際に描画する(後々Index描画に変える)
	magosuya_->GetDxCommon ()->GetCommandList ()->DrawInstanced (static_cast<UINT>(modelData_.vertexCount), 1, 0, 0);
}