#include "LineRenderer.h"
#include "MathFunction.h"
#include "MaxMeshNum.h"

LineRenderer::~LineRenderer () {

}

void LineRenderer::Initialize (DxCommon* dxCommon) {
	dxCommon_ = dxCommon;
	device_ = dxCommon->GetDevice ();
	commandList_ = dxCommon->GetCommandList ();

	lineBuffer_ = std::make_unique<LineData> ();

	//頂点バッファー作成とマッピング
	lineBuffer_->vertexBuffer = dxCommon_->CreateBufferResource (sizeof (LineVertexData) * VertexNum::Line * MaxMeshNum::Line);
	lineBuffer_->vertexBuffer->Map (0, nullptr, reinterpret_cast<void**>(&vertexData_));
	lineBuffer_->vbView.BufferLocation = lineBuffer_->vertexBuffer->GetGPUVirtualAddress ();
	lineBuffer_->vbView.SizeInBytes = sizeof (LineVertexData) * VertexNum::Line * MaxMeshNum::Line;
	lineBuffer_->vbView.StrideInBytes = sizeof (LineVertexData);

	//行列バッファー作成とマッピング
	instancingBuffer_ = dxCommon_->CreateBufferResource (sizeof (LineForGPU) * VertexNum::Line * MaxMeshNum::Line);
	instancingBuffer_->Map (0, nullptr, reinterpret_cast<void**>(&instancingData_));
	for (uint32_t i = 0; i < VertexNum::Line * MaxMeshNum::Line; ++i) {
		instancingData_[i].World = MakeIdentity4x4 ();
		instancingData_[i].WVP = MakeIdentity4x4 ();
	}

	//instancing用にSRVを作成
	// LineRenderer.h に SRVハンドル用のメンバを追加
	D3D12_CPU_DESCRIPTOR_HANDLE lineSrvHandleCPU = {};
	D3D12_GPU_DESCRIPTOR_HANDLE lineSrvHandleGPU = {};

	// LineRenderer.cpp::Initialize 内で作成
	D3D12_SHADER_RESOURCE_VIEW_DESC lineSrvDesc = {};
	lineSrvDesc.Format = DXGI_FORMAT_UNKNOWN; // Structured BufferなのでUNKNOWN
	lineSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	lineSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	lineSrvDesc.Buffer.FirstElement = 0;
	lineSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	lineSrvDesc.Buffer.NumElements = MaxMeshNum::Line; // 線の最大数
	lineSrvDesc.Buffer.StructureByteStride = sizeof (LineForGPU); // 1インスタンスのサイズ

	lineSrvHandleCPU = dxCommon_->GetCPUDescriptorHandle (
		dxCommon_->GetsrvDescriptorHeap (), dxCommon_->GetDescriptorSizeSrv (), descriptorIndex_
	);
	lineSrvHandleGPU = dxCommon_->GetGPUDescriptorHandle (
		dxCommon_->GetsrvDescriptorHeap (), dxCommon_->GetDescriptorSizeSrv (), descriptorIndex_
	);
	device_->CreateShaderResourceView (instancingBuffer_.Get (), &lineSrvDesc, lineSrvHandleCPU);

	//PSOの設定
	desc_.RootSignatureID = RootSignatureManager::GetInstance ()->GetOrCreateRootSignature (RootSigType::Line);
	desc_.VS_ID = ShaderManager::GetInstance ()->CompileAndCasheShader (L"Resources/shader/line.VS.hlsl", L"vs_6_0");
	desc_.PS_ID = ShaderManager::GetInstance ()->CompileAndCasheShader (L"Resources/shader/line.PS.hlsl", L"ps_6_0");
	desc_.InputLayoutID = InputLayoutType::Line;
	desc_.BlendMode = BlendModeType::Opaque;
	desc_.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;	//線を描画
	PSOManager::GetInstance ()->GetOrCreratePSO (desc_);
}

void LineRenderer::UpdateVertexData (const LineVertexData* data) {
	//currentLineNumが最大数を超えていないか
	if (currentLineCount_ >= MaxMeshNum::Line) {
		//超えてたら早期リターン
		return;
	}

	// 現在の描画位置のオフセット (線1本 = 2頂点)
	uint32_t offset = currentLineCount_ * VertexNum::Line;

	//オフセットを使ってライン描画に必要な分だけバッファーにコピー
	vertexData_[offset + 0] = data[0];
	vertexData_[offset + 1] = data[1];

	//線の数をインクリメント
	currentLineCount_++;
}

void LineRenderer::TransferData (const LineForGPU* data) {
	for (uint32_t i = 0; i < VertexNum::Line * MaxMeshNum::Line; ++i) {
		instancingData_[i].World = data[i].World;
		instancingData_[i].WVP = data[i].WVP;
	}
}

void LineRenderer::Draw () {
	//RootSignatureとPSOをセット
	RootSignatureManager::GetInstance ()->SetRootSignature (desc_.RootSignatureID);
	PSOManager::GetInstance ()->SetPSO (desc_);
	commandList_->IASetPrimitiveTopology (D3D_PRIMITIVE_TOPOLOGY_LINELIST);							//線で描画
	commandList_->IASetVertexBuffers (0, 1, &lineBuffer_->vbView);									//VBVを設定
	commandList_->SetGraphicsRootConstantBufferView (0, instancingBuffer_->GetGPUVirtualAddress ());	//CBVをセット
	// 実際に描画する頂点数 = 現在の線分数 * 2頂点
	uint32_t drawVertexCount = currentLineCount_ * VertexNum::Line;

	if (drawVertexCount > 0) {
		// 描画
		commandList_->DrawInstanced (drawVertexCount, 1, 0, 0); // 描画要求があった頂点数だけ描画
	}

	//次フレームのためにカウントをリセット
	ResetCurrentLineCount ();
}