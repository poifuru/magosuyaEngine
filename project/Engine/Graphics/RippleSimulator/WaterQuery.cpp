#include "PCH.h"
#include "WaterQuery.h"
#include "Function.h"
#include "ShaderManager.h"
#include "SrvDescriptorHeapPool.h"	

void WaterQuery::Initialize(
	ID3D12Device* device,						
	MyEngine::LowLevel::SrvDescriptorHeapPool& heapManager,
	MyEngine::Rendering::ShaderManager& shaderManager,
	ID3D12RootSignature* computeRootSignature
) {
	heapManager_ = &heapManager;
	rootSig_ = computeRootSignature;
	const UINT64 bufferSize = sizeof(WaterQueryResult) * kMaxQueries;

	// --- 結果バッファ（DEFAULT ヒープ、UAV可） ---
	D3D12_HEAP_PROPERTIES defaultHeap{};
	defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = bufferSize;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	HRESULT hr = device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &desc,
												 D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(resultBuffer_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	// --- リードバックバッファ（READBACK ヒープ） ---
	// READBACK ヒープのバッファは COPY_DEST で作り、UAVフラグは付けない
	D3D12_HEAP_PROPERTIES readbackHeap{};
	readbackHeap.Type = D3D12_HEAP_TYPE_READBACK;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	hr = device->CreateCommittedResource(&readbackHeap, D3D12_HEAP_FLAG_NONE, &desc,
										 D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(readbackBuffer_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	// --- RWStructuredBuffer 用の UAV ---
	// CreateUAVforTexture2D は中身が CreateUnorderedAccessView を呼ぶだけなので、バッファにも使える
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = kMaxQueries;
	uavDesc.Buffer.StructureByteStride = sizeof(WaterQueryResult);
	uavIndex_ = heapManager.AllocateIndex();
	heapManager.CreateUAVforTexture2D(uavIndex_, resultBuffer_.Get(), uavDesc);

	paramBuffer_.Initialize(device);

	// --- PSO ---
	uint32_t csID = shaderManager.CompileAndCacheShader(L"Resources/Shader/WaterQuery.CS.hlsl", L"cs_6_0");
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = rootSig_;
	psoDesc.CS = shaderManager.GetShaderBytecode(csID);
	hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(pso_.GetAddressOf()));
	assert(SUCCEEDED(hr));
}

void WaterQuery::SetQueryPoint(uint32_t slot, float x, float z) {
	if (slot >= kMaxQueries) return;
	params_.points[slot] = Vector4(x, z, 0.0f, 0.0f);
	usedSlots_ = (std::max)(usedSlots_, slot + 1);
}

const WaterQueryResult& WaterQuery::GetResult(uint32_t slot) {
	ReadBackIfNeeded();
	static const WaterQueryResult kEmpty{};
	return (slot < kMaxQueries) ? results_[slot] : kEmpty;
}

void WaterQuery::Dispatch(ID3D12GraphicsCommandList* cmdList, uint32_t rippleSrvIndex,
						  const Vector2& waterMin, const Vector2& waterSize) {

	// 読む前に次の Dispatch が来たら、ここで前の結果を確定させておく
	ReadBackIfNeeded();
	if (usedSlots_ == 0) return;

	params_.waterMin = waterMin;
	params_.waterSize = waterSize;
	params_.rippleTextureIndex = rippleSrvIndex;
	params_.count = usedSlots_;
	paramBuffer_.Update(params_);

	cmdList->SetComputeRootSignature(rootSig_);
	cmdList->SetPipelineState(pso_.Get());
	cmdList->SetComputeRootDescriptorTable(0, heapManager_->GetGpuHandle(uavIndex_)); // u0
	cmdList->SetComputeRootConstantBufferView(1, paramBuffer_.GetGPUVirtualAddress()); // b0
	cmdList->SetComputeRootDescriptorTable(2, heapManager_->GetGpuHandle(0));          // t0 space2
	cmdList->Dispatch((usedSlots_ + 15) / 16, 1, 1);

	// UAV → コピー元 → リードバックへコピー → UAVに戻す
	MyEngine::Utility::TransitionBarrier(cmdList, resultBuffer_.Get(),
										 D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	cmdList->CopyBufferRegion(readbackBuffer_.Get(), 0, resultBuffer_.Get(), 0,
							  sizeof(WaterQueryResult) * usedSlots_);
	MyEngine::Utility::TransitionBarrier(cmdList, resultBuffer_.Get(),
										 D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	hasPendingCopy_ = true;
}

void WaterQuery::ReadBackIfNeeded() {
	// EndFrame で GPU の完了を待っているので、次のフレームなら安全に読める
	if (!hasPendingCopy_) return;

	D3D12_RANGE readRange{ 0, sizeof(WaterQueryResult) * usedSlots_ };
	void* mapped = nullptr;
	if (SUCCEEDED(readbackBuffer_->Map(0, &readRange, &mapped))) {
		std::memcpy(results_, mapped, sizeof(WaterQueryResult) * usedSlots_);
		D3D12_RANGE writeRange{ 0, 0 }; // CPU からは何も書いていない
		readbackBuffer_->Unmap(0, &writeRange);
	}
	hasPendingCopy_ = false;
}