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
	uvTransform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	//乱数エンジンのインスタンスを作成してrdの結果で初期化する
	randomEngine_.seed (rd ());
	emitter_.count = 20;
	emitter_.frequency = 0.3f;
	emitter_.frequencyTime = 0.0f;
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
	//desc_.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

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
		dxCommon_->GetsrvDescriptorHeap (), dxCommon_->GetDescriptorSizeSrv (), descriptorIndex_
	);
	particleSrvHandleGPU = dxCommon_->GetGPUDescriptorHandle (
		dxCommon_->GetsrvDescriptorHeap (), dxCommon_->GetDescriptorSizeSrv (), descriptorIndex_
	);
	device_->CreateShaderResourceView (instancingBuffer_.Get (), &particleSrvDesc, particleSrvHandleCPU);

	billBoardMatrix_ = MakeIdentity4x4 ();

	descriptorIndex_++;
}

void Particle::Update (Matrix4x4* cameraMatrix, Matrix4x4* vp) {
	//Emitter更新
	EmitterUpdate ();

	//numInstanceのリセット
	numInstance_ = 0;
	uint32_t dstIndex = 0; //書き込み先インデックス
	for (particleIterator_ = particles_.begin (); particleIterator_ != particles_.end ();) {
		//生存可能時間を過ぎていたら更新処理をしない
		if (particleIterator_->lifeTime <= particleIterator_->currentTime) {
			particleIterator_ = particles_.erase (particleIterator_);	//生存時間をすぎたパーティクルはリストから削除
			continue;
		}

		if (dstIndex < kMaxParticleNum_) {
			//速度を反映させる
			particleIterator_->transform.translate += particleIterator_->velocity * kDeltaTime;
			particleIterator_->currentTime += kDeltaTime;
			instancingData_[dstIndex].World = MakeAffineMatrix (
				particleIterator_->transform.scale,
				particleIterator_->transform.rotate,
				particleIterator_->transform.translate
			);

			//ビルボードフラグが立っていたら
			if (useBillBoard) {
				//カメラのWorld行列の回転成分だけを使う
				billBoardMatrix_ = *cameraMatrix;
				billBoardMatrix_.m[3][0] = 0.0f;	//平行移動成分はいらない
				billBoardMatrix_.m[3][1] = 0.0f;
				billBoardMatrix_.m[3][2] = 0.0f;
				billBoardMatrix_.m[3][3] = 1.0f;

				instancingData_[dstIndex].World = Multiply (instancingData_[dstIndex].World, billBoardMatrix_);
				instancingData_[dstIndex].WVP = Multiply (instancingData_[dstIndex].World, *vp);
			}
			else {
				instancingData_[dstIndex].WVP = Multiply (instancingData_[dstIndex].World, *vp);
			}
			instancingData_[dstIndex].color = particleIterator_->color;
			//徐々に透明度を下げて消えるように
			float alpha = 1.0f - (particleIterator_->currentTime / particleIterator_->lifeTime);
			instancingData_[dstIndex].color.w = alpha;

			//uvTranform更新
			materialData_[dstIndex].uvTranform = MakeAffineMatrix (
				uvTransform_.scale,
				uvTransform_.rotate,
				uvTransform_.translate);
		}

		//書き込み先インデックスをインクリメント
		++dstIndex;
		++particleIterator_;
	}
	numInstance_ = dstIndex;
}

void Particle::Draw () {
	RootSignatureManager::GetInstance ()->SetRootSignature (desc_.RootSignatureID);
	PSOManager::GetInstance ()->SetPSO (desc_);
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
	}

	if (ImGui::Button ("spawn")) {
		particles_.splice (particles_.end (), Emit (emitter_, randomEngine_));
	}

	ImGui::Checkbox ("useBillBoard", &useBillBoard);

	ImGui::Separator ();

	ImGui::DragFloat3 ("scale", &emitter_.transform.scale.x, 0.01f, 0.0f, 100.f);
	ImGui::DragFloat3 ("rotate", &emitter_.transform.rotate.x, 0.01f, -100.0f, 100.f);
	ImGui::DragFloat3 ("translate", &emitter_.transform.translate.x, 0.01f, -100.0f, 100.f);

	ImGui::End ();
}

ParticleData Particle::MakeNewParticle (std::mt19937 randomEngine, const Emitter& emitter_) {
	//乱数エンジンのインスタンスを作成してrdの結果で初期化する
	randomEngine.seed (rd ());

	ParticleData data;

	//使う分布を初期化する
	pos_x = std::uniform_real_distribution<float> (-emitter_.transform.scale.x, emitter_.transform.scale.x);
	pos_y = std::uniform_real_distribution<float> (-emitter_.transform.scale.y, emitter_.transform.scale.y);
	pos_z = std::uniform_real_distribution<float> (-emitter_.transform.scale.z, emitter_.transform.scale.z);
	rand_ = std::uniform_real_distribution<float> (-1.0f, 1.0f);
	randColor_ = std::uniform_real_distribution<float> (0.0f, 1.0f);
	randTime_ = std::uniform_real_distribution<float> (1.0f, 3.0f);

	//パーティクル情報の初期化
	data.transform.scale = { 1.0f, 1.0f, 1.0f };
	data.transform.rotate = { 0.0f, 0.0f, 0.0f };
	data.transform.translate = { pos_x (randomEngine), pos_y (randomEngine), pos_z (randomEngine) };
	data.velocity = { rand_ (randomEngine), rand_ (randomEngine), rand_ (randomEngine) };
	data.color = { randColor_ (randomEngine), randColor_ (randomEngine), randColor_ (randomEngine), 1.0f };
	data.lifeTime = randTime_ (randomEngine);
	data.currentTime = 0.0f;

	//emitterを加味してtranslateを再計算
	data.transform.translate += emitter_.transform.translate;

	return data;
}

std::list<ParticleData> Particle::Emit (const Emitter& emitter, std::mt19937& randomEngine) {
	std::list<ParticleData> particles;
	for (uint32_t count = 0; count < emitter.count; ++count) {
		particles.push_back (MakeNewParticle (randomEngine, emitter_));
	}
	return particles;
}

void Particle::EmitterUpdate () {
	emitter_.frequencyTime += kDeltaTime;	//発生時刻を進める
	if (emitter_.frequency <= emitter_.frequencyTime) {		//頻度より大きいなら
		particles_.splice (particles_.end (), Emit (emitter_, randomEngine_));	//particle発生
		emitter_.frequencyTime -= emitter_.frequency;	//進めた時間を戻す
	}
}