#include "Particle.h"
#include "MagosuyaEngine.h"
#include "MathFunction.h"

Particle::Particle (MagosuyaEngine* magosuya) {
	magosuya_ = magosuya;
	data_ = std::make_unique<ModelData> ();
	transform_ = { {1.0f, 1.0f, 1.0f},{0.0f, 0.0f, 0.0f},{0.0f, 0.0f, 0.0f} };
	uvTransform_ = { {1.0f, 1.0f, 1.0f},{0.0f, 0.0f, 0.0f},{0.0f, 0.0f, 0.0f} };
}

Particle::~Particle () {

}

void Particle::Initialize () {
	data_->vertexBuffer = magosuya_->GetDxCommon ()->CreateBufferResource (sizeof (VertexData) * 4);
	// 💡 正しいポインタ（vertexData_）にMapで取得したアドレスを書き込むでやんす！
	data_->vertexBuffer->Map (0, nullptr, reinterpret_cast<void**>(&vertexData_));
	data_->vbView.BufferLocation = data_->vertexBuffer->GetGPUVirtualAddress ();
	data_->vbView.SizeInBytes = sizeof (VertexData) * 4;
	data_->vbView.StrideInBytes = sizeof (VertexData);

	// インデックスバッファ作成
	data_->indexBuffer = magosuya_->GetDxCommon ()->CreateBufferResource (sizeof (uint32_t) * 6);
	// 💡 正しいポインタ（indexData_）にMapで取得したアドレスを書き込むでやんす！
	data_->indexBuffer->Map (0, nullptr, reinterpret_cast<void**>(&indexData_));
	data_->ibView.BufferLocation = data_->indexBuffer->GetGPUVirtualAddress ();
	data_->ibView.SizeInBytes = sizeof (uint32_t) * 6;
	data_->ibView.Format = DXGI_FORMAT_R32_UINT;

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

	//vertexData_に書き込み
	//左上
	vertexData_[0] = { { -1.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } };
	//右上
	vertexData_[1] = { { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } };
	//左下
	vertexData_[2] = { { -1.0f, -1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } };
	//右下
	vertexData_[3] = { { 1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } };

	//indexData_に書き込み
	indexData_[0] = 0;
	indexData_[1] = 1;
	indexData_[2] = 2;
	indexData_[3] = 1;
	indexData_[4] = 3;
	indexData_[5] = 2;
}

void Particle::Update (Matrix4x4* vp) {
	matrixData_->World = MakeAffineMatrix (transform_.scale, transform_.rotate, transform_.translate);
	matrixData_->WVP = Multiply (matrixData_->World, *vp);
	matrixData_->WorldInverseTranspose = Transpose (Inverse (matrixData_->World));

	//uvTranform更新
	materialData_->uvTranform = MakeAffineMatrix (uvTransform_.scale, uvTransform_.rotate, uvTransform_.translate);
}

void Particle::Draw () {
	magosuya_->GetDxCommon ()->GetCommandList ()->IASetPrimitiveTopology (D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	magosuya_->GetDxCommon ()->GetCommandList ()->IASetVertexBuffers (0, 1, &data_->vbView);   //VBVを設定
	magosuya_->GetDxCommon ()->GetCommandList ()->IASetIndexBuffer (&data_->ibView);	        //IBVを設定
	magosuya_->GetDxCommon ()->GetCommandList ()->SetGraphicsRootConstantBufferView (0, matrixBuffer_->GetGPUVirtualAddress ());
	magosuya_->GetDxCommon ()->GetCommandList ()->SetGraphicsRootConstantBufferView (1, materialBuffer_->GetGPUVirtualAddress ());
	magosuya_->GetDxCommon ()->GetCommandList ()->SetGraphicsRootDescriptorTable (2, handle_);
	//インデックスバッファを使った描画
	magosuya_->GetDxCommon ()->GetCommandList ()->DrawIndexedInstanced (6, 1, 0, 0, 0);
}