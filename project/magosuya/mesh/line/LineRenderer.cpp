#include "LineRenderer.h"
#include "MathFunction.h"
#include "MaxMeshNum.h"
#include "DxCommon.h"

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

	//行列バッファー作成とマッピング(頂点2つにつき1つ)
	instancingBuffer_ = dxCommon_->CreateBufferResource (sizeof (LineForGPU) * MaxMeshNum::Line);
	instancingBuffer_->Map (0, nullptr, reinterpret_cast<void**>(&instancingData_));
	for (uint32_t i = 0; i < MaxMeshNum::Line; ++i) {
		instancingData_[i].World = MakeIdentity4x4 ();
		instancingData_[i].WVP = MakeIdentity4x4 ();
	}

	//instancing用にSRVを作成(t0にバインド)
	D3D12_SHADER_RESOURCE_VIEW_DESC lineSrvDesc = {};
	lineSrvDesc.Format = DXGI_FORMAT_UNKNOWN; // Structured BufferなのでUNKNOWN
	lineSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	lineSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	lineSrvDesc.Buffer.FirstElement = 0;
	lineSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	lineSrvDesc.Buffer.NumElements = MaxMeshNum::Line; // 線の最大数
	lineSrvDesc.Buffer.StructureByteStride = sizeof (LineForGPU); // 1インスタンスのサイズ

	lineSrvHandleCPU_ = dxCommon_->GetCPUDescriptorHandle (
		dxCommon_->GetsrvDescriptorHeap (), dxCommon_->GetDescriptorSizeSrv (), descriptorIndex_
	);
	lineSrvHandleGPU_ = dxCommon_->GetGPUDescriptorHandle (
		dxCommon_->GetsrvDescriptorHeap (), dxCommon_->GetDescriptorSizeSrv (), descriptorIndex_
	);
	device_->CreateShaderResourceView (instancingBuffer_.Get (), &lineSrvDesc, lineSrvHandleCPU_);

	// 頂点バッファ用のSRVを作成(t1にバインド)
	D3D12_SHADER_RESOURCE_VIEW_DESC vertexSrvDesc = {};
	vertexSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	vertexSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	vertexSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	vertexSrvDesc.Buffer.FirstElement = 0;
	vertexSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	vertexSrvDesc.Buffer.NumElements = VertexNum::Line * MaxMeshNum::Line; // 全頂点数
	vertexSrvDesc.Buffer.StructureByteStride = sizeof (LineVertexData); // 1頂点のサイズ

	// LineForGPUのSRV(t0)の後に、頂点バッファのSRV(t1)をセット
	// descriptorIndex_ は t0 として使うでやんす
	const uint32_t vertexDescriptorIndex_ = descriptorIndex_ + 1;

	vertexSrvHandleCPU_ = dxCommon_->GetCPUDescriptorHandle (
		dxCommon_->GetsrvDescriptorHeap (), dxCommon_->GetDescriptorSizeSrv (), vertexDescriptorIndex_
	);
	vertexSrvHandleGPU_ = dxCommon_->GetGPUDescriptorHandle (
		dxCommon_->GetsrvDescriptorHeap (), dxCommon_->GetDescriptorSizeSrv (), vertexDescriptorIndex_
	);
	device_->CreateShaderResourceView (lineBuffer_->vertexBuffer.Get (), &vertexSrvDesc, vertexSrvHandleCPU_);

	//PSOの設定
	desc_.RootSignatureID = RootSignatureManager::GetInstance ()->GetOrCreateRootSignature (RootSigType::LineMesh);
	desc_.VS_ID = ShaderManager::GetInstance ()->CompileAndCasheShader (L"Resources/shader/Line.VS.hlsl", L"vs_6_0");
	desc_.PS_ID = ShaderManager::GetInstance ()->CompileAndCasheShader (L"Resources/shader/Line.PS.hlsl", L"ps_6_0");
	desc_.InputLayoutID = InputLayoutType::LineMesh;
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

void LineRenderer::TransferData (const LineForGPU& data) {
	//描画要求数が0か最大数を超えてるなら
	if (currentLineCount_ == 0 || currentLineCount_ > MaxMeshNum::Line) {
		//早期リターン
		return;
	}

	// 書き込むインスタンス配列のインデックス
	uint32_t instanceIndex = currentLineCount_ - 1;

	instancingData_[instanceIndex].World = data.World;
	instancingData_[instanceIndex].WVP = data.WVP;
}

void LineRenderer::Draw () {
	//RootSignatureとPSOをセット
	RootSignatureManager::GetInstance ()->SetRootSignature (desc_.RootSignatureID);
	PSOManager::GetInstance ()->SetPSO (desc_);
	commandList_->IASetPrimitiveTopology (D3D_PRIMITIVE_TOPOLOGY_LINELIST);							//線で描画									//VBVを設定
	commandList_->SetGraphicsRootConstantBufferView (0, instancingBuffer_->GetGPUVirtualAddress ());	//CBVをセット
	commandList_->SetGraphicsRootDescriptorTable (1, lineSrvHandleGPU_);

	if (currentLineCount_ > 0) {
		// 描画
		commandList_->DrawInstanced (VertexNum::Line, currentLineCount_, 0, 0); // 描画要求があった頂点数だけ描画
	}

	//次フレームのためにカウントをリセット
	ResetCurrentLineCount ();
}