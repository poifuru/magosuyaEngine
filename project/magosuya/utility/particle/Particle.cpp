#include "Particle.h"
#include "DxCommon.h"
#include "MathFunction.h"

Particle::Particle (DxCommon* dxCommon) {
	dxCommon_ = dxCommon;
	commandList_ = dxCommon->GetCommandList ();
	data_ = std::make_unique<ModelData> ();
	transform_ = { {1.0f, 1.0f, 1.0f},{0.0f, 0.0f, 0.0f},{0.0f, 0.0f, 0.0f} };
	uvTransform_ = { {1.0f, 1.0f, 1.0f},{0.0f, 0.0f, 0.0f},{0.0f, 0.0f, 0.0f} };
}

Particle::~Particle () {

}

void Particle::Initialize () {
	//頂点バッファ作成
	data_->vertexBuffer = dxCommon_->CreateBufferResource (sizeof (VertexData) * 4);
	data_->vertexBuffer->Map (0, nullptr, reinterpret_cast<void**>(&vertexData_));
	data_->vbView.BufferLocation = data_->vertexBuffer->GetGPUVirtualAddress ();
	data_->vbView.SizeInBytes = sizeof (VertexData) * 4;
	data_->vbView.StrideInBytes = sizeof (VertexData);

	//インデックスバッファ作成
	data_->indexBuffer = dxCommon_->CreateBufferResource (sizeof (uint32_t) * 6);
	data_->indexBuffer->Map (0, nullptr, reinterpret_cast<void**>(&indexData_));
	data_->ibView.BufferLocation = data_->indexBuffer->GetGPUVirtualAddress ();
	data_->ibView.SizeInBytes = sizeof (uint32_t) * 6;
	data_->ibView.Format = DXGI_FORMAT_R32_UINT;

	//行列データ
	matrixBuffer_ = dxCommon_->CreateBufferResource (sizeof (TransformationMatrix));
	matrixBuffer_->Map (0, nullptr, reinterpret_cast<void**>(&matrixData_));
	matrixData_->World = MakeIdentity4x4 ();
	matrixData_->WVP = MakeIdentity4x4 ();
	matrixData_->WorldInverseTranspose = MakeIdentity4x4 ();

	//マテリアルデータ
	materialBuffer_ = dxCommon_->CreateBufferResource (sizeof (Material));
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

	//PSO設定
	desc_.RootSignatureID = RootSignatureManager::GetInstance ()->GetOrCreateRootSignature (RootSigType::Standard3D);
	desc_.VS_ID = ShaderManager::GetInstance ()->CompileAndCasheShader (L"Resources/shader/Object3d.VS.hlsl", L"vs_6_0");
	desc_.PS_ID = ShaderManager::GetInstance ()->CompileAndCasheShader (L"Resources/shader/Object3d.PS.hlsl", L"ps_6_0");
	desc_.InputLayoutID = InputLayoutType::Standard3D;
	desc_.BlendMode = BlendModeType::Opaque;
	rootsignature_ = RootSignatureManager::GetInstance ()->GetRootSignature (desc_.RootSignatureID);
	pipelineState_ = PSOManager::GetInstance ()->GetOrCreratePSO (desc_);
}

void Particle::Update (Matrix4x4* vp) {
	matrixData_->World = MakeAffineMatrix (transform_.scale, transform_.rotate, transform_.translate);
	matrixData_->WVP = Multiply (matrixData_->World, *vp);
	matrixData_->WorldInverseTranspose = Transpose (Inverse (matrixData_->World));

	//uvTranform更新
	materialData_->uvTranform = MakeAffineMatrix (uvTransform_.scale, uvTransform_.rotate, uvTransform_.translate);
}

void Particle::Draw () {
	commandList_->SetGraphicsRootSignature (rootsignature_);
	assert (rootsignature_ != nullptr);
	commandList_->SetPipelineState (pipelineState_);
	assert (pipelineState_ != nullptr);
	commandList_->IASetPrimitiveTopology (D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList_->IASetVertexBuffers (0, 1, &data_->vbView);   //VBVを設定
	commandList_->IASetIndexBuffer (&data_->ibView);	        //IBVを設定
	commandList_->SetGraphicsRootConstantBufferView (0, matrixBuffer_->GetGPUVirtualAddress ());
	commandList_->SetGraphicsRootConstantBufferView (1, materialBuffer_->GetGPUVirtualAddress ());
	commandList_->SetGraphicsRootDescriptorTable (2, handle_);
	//インデックスバッファを使った描画
	commandList_->DrawIndexedInstanced (6, 1, 0, 0, 0);
}