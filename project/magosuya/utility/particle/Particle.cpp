#include <imgui.h>
#include <numbers>
#include "Particle.h"
#include "DxCommon.h"
#include "MathFunction.h"

//デルタタイムを定義
const float kDeltaTime = 1.0f / 60.0f;

Particle::Particle (DxCommon* dxCommon) {
	dxCommon_ = dxCommon;
	device_ = dxCommon->GetDevice ();
	commandList_ = dxCommon->GetCommandList ();
	data_ = std::make_unique<ModelData> ();
	for (uint32_t i = 0; i < kMaxParticleNum_; ++i) {
		particles_[i].transform = { {1.0f, 1.0f, 1.0f},{0.0f, 0.0f, 0.0f},{0.0f, 0.0f, 0.0f} };
		uvTransform_[i] = { {1.0f, 1.0f, 1.0f},{0.0f, 0.0f, 0.0f},{0.0f, 0.0f, 0.0f} };
	}
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

	//Instancing用の行列データ
	instancingBuffer_ = dxCommon_->CreateBufferResource (sizeof (ParticleForGPU) * kMaxParticleNum_);
	instancingBuffer_->Map (0, nullptr, reinterpret_cast<void**>(&instancingData_));
	//マテリアルデータ
	materialBuffer_ = dxCommon_->CreateBufferResource (sizeof (Material) * kMaxParticleNum_);
	materialBuffer_->Map (0, nullptr, reinterpret_cast<void**> (&materialData_));
	for (uint32_t i = 0; i < kMaxParticleNum_; ++i) {
		instancingData_[i].WVP = MakeIdentity4x4 ();
		instancingData_[i].World = MakeIdentity4x4 ();
		instancingData_[i].color = { 1.0f, 1.0f, 1.0f, 1.0f };
		materialData_[i].color = { 1.0f, 1.0f, 1.0f, 1.0f };
		materialData_[i].enableLighting = false;
		materialData_[i].uvTranform = MakeIdentity4x4 ();
	}

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
	desc_.RootSignatureID = RootSignatureManager::GetInstance ()->GetOrCreateRootSignature (RootSigType::Particle);
	desc_.VS_ID = ShaderManager::GetInstance ()->CompileAndCasheShader (L"Resources/shader/Particle.VS.hlsl", L"vs_6_0");
	desc_.PS_ID = ShaderManager::GetInstance ()->CompileAndCasheShader (L"Resources/shader/Particle.PS.hlsl", L"ps_6_0");
	desc_.InputLayoutID = InputLayoutType::Particle;
	desc_.BlendMode = BlendModeType::Additive;
	desc_.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;	//Depthの書き込みを行わない
	rootsignature_ = RootSignatureManager::GetInstance ()->GetRootSignature (desc_.RootSignatureID);
	pipelineState_ = PSOManager::GetInstance ()->GetOrCreratePSO (desc_);

	//particle用SRVを作成する
	D3D12_SHADER_RESOURCE_VIEW_DESC particleSrvDesc = {};
	particleSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	particleSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	particleSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	particleSrvDesc.Buffer.FirstElement = 0;
	particleSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	particleSrvDesc.Buffer.NumElements = kMaxParticleNum_;
	particleSrvDesc.Buffer.StructureByteStride = sizeof (ParticleForGPU);
	particleSrvHandleCPU = dxCommon_->GetCPUDescriptorHandle (
		dxCommon_->GetsrvDescriptorHeap (), dxCommon_->GetDescriptorSizeSrv (), 3
	);
	particleSrvHandleGPU = dxCommon_->GetGPUDescriptorHandle (
		dxCommon_->GetsrvDescriptorHeap (), dxCommon_->GetDescriptorSizeSrv (), 3
	);
	device_->CreateShaderResourceView (instancingBuffer_.Get (), &particleSrvDesc, particleSrvHandleCPU);

	//particeDataの生成
	for (uint32_t i = 0; i < kMaxParticleNum_; ++i) {
		//乱数エンジンのインスタンスを作成してrdの結果で初期化する
		randomEngine_.seed (rd ());
		particles_[i] = MakeNewParticle (randomEngine_);
	}

	billBoardMatrix_ = MakeIdentity4x4 ();
}

void Particle::Update (Matrix4x4* cameraMatrix, Matrix4x4* vp) {
	//numInstanceのリセット
	numInstance_ = 0;
	uint32_t dstIndex = 0; //書き込み先インデックス
	for (uint32_t i = 0; i < kMaxParticleNum_; ++i) {
		//生存可能時間を過ぎていたら更新処理をしない
		if (particles_[i].lifeTime <= particles_[i].currentTime) {
			continue;
		}

		//速度を反映させる
		particles_[i].transform.translate += particles_[i].velocity * kDeltaTime;
		particles_[i].currentTime += kDeltaTime;
		instancingData_[dstIndex].World = MakeAffineMatrix (particles_[i].transform.scale, particles_[i].transform.rotate, particles_[i].transform.translate);
		if (useBillBoard) {
			billBoardMatrix_ = Multiply (billBoardMatrix_, *cameraMatrix);
			billBoardMatrix_.m[3][0] = 0.0f;	//平行移動成分はいらない
			billBoardMatrix_.m[3][1] = 0.0f;
			billBoardMatrix_.m[3][2] = 0.0f;

			instancingData_[dstIndex].World = Multiply (instancingData_[dstIndex].World, billBoardMatrix_);
		}
		instancingData_[dstIndex].WVP = Multiply (instancingData_[dstIndex].World, *vp);
		instancingData_[dstIndex].color = particles_[i].color;
		//徐々に透明度を下げて消えるように
		float alpha = 1.0f - (particles_[i].currentTime / particles_[i].lifeTime);
		instancingData_[dstIndex].color.w = alpha;

		//uvTranform更新
		materialData_[dstIndex].uvTranform = MakeAffineMatrix (uvTransform_[i].scale, uvTransform_[i].rotate, uvTransform_[i].translate);
		
		//書き込み先インデックスをインクリメント
		++dstIndex;
	}
	numInstance_ = dstIndex;
}

void Particle::Draw () {
	commandList_->SetGraphicsRootSignature (rootsignature_);
	assert (rootsignature_ != nullptr);
	commandList_->SetPipelineState (pipelineState_);
	assert (pipelineState_ != nullptr);
	commandList_->IASetPrimitiveTopology (D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList_->IASetVertexBuffers (0, 1, &data_->vbView);   //VBVを設定
	commandList_->IASetIndexBuffer (&data_->ibView);	        //IBVを設定
	commandList_->SetGraphicsRootDescriptorTable (0, particleSrvHandleGPU);
	commandList_->SetGraphicsRootConstantBufferView (1, materialBuffer_->GetGPUVirtualAddress ());
	commandList_->SetGraphicsRootDescriptorTable (2, handle_);
	//インデックスバッファを使った描画
	commandList_->DrawIndexedInstanced (6, numInstance_, 0, 0, 0);
}

void Particle::ImGui () {
	//BlendMode切り替え
	ImGui::Begin ("Particle");
	if (ImGui::Combo ("BlendMode", &currentBlendMode_, blendModeNames_, kBlendModeCount_)) {
		desc_.BlendMode = static_cast<BlendModeType>(currentBlendMode_);
		pipelineState_ = PSOManager::GetInstance ()->GetOrCreratePSO (desc_);
		commandList_->SetPipelineState (pipelineState_);
		assert (pipelineState_ != nullptr);
	}

	if (ImGui::Button ("respawn")) {
		//particeDataの生成
		for (uint32_t i = 0; i < kMaxParticleNum_; ++i) {
			//乱数エンジンのインスタンスを作成してrdの結果で初期化する
			randomEngine_.seed (rd ());
			particles_[i] = MakeNewParticle (randomEngine_);
		}
	}

	if (ImGui::Checkbox ("useBillBoard", &useBillBoard)) {
		
	}

	ImGui::End ();
}

ParticleData Particle::MakeNewParticle (std::mt19937 randomEngine) {
	ParticleData data;

	//使う分布を初期化する
	rand_ = std::uniform_real_distribution<float> (-0.05f, 0.05f);
	randColor_ = std::uniform_real_distribution<float> (0.0f, 1.0f);
	randTime_ = std::uniform_real_distribution<float> (10.0f, 30.0f);

	//Transformの初期化(ちょっとづつずらして配置)
	data.transform.scale = { 1.0f, 1.0f, 1.0f };
	data.transform.rotate = { 0.0f, 0.0f, 0.0f };
	data.transform.translate = { rand_ (randomEngine), rand_ (randomEngine), rand_ (randomEngine) };
	data.velocity = { rand_ (randomEngine), rand_ (randomEngine), rand_ (randomEngine) };
	data.color = { randColor_ (randomEngine), randColor_ (randomEngine), randColor_ (randomEngine), 1.0f };
	data.lifeTime = randTime_ (randomEngine);
	data.currentTime = 0.0f;

	return data;
}