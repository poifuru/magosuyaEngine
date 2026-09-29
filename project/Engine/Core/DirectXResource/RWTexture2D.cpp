#include "PCH.h"
#include "RWTexture2D.h"
#include "SrvDescriptorHeap.h"
#include "Function.h"

void RWTexture2D::Initialize(
	ID3D12Device* device, 
	MyEngine::LowLevel::SrvDescriptorHeap& heapManager,
	uint32_t width,
	uint32_t height,
	DXGI_FORMAT format
) {
	Release();
	assert(device != nullptr);

	heapManager_ = &heapManager;
	width_ = width;
	height_ = height;
	format_ = format;

	// GPU専用メモリ(DEFAULT)にUAV許可フラグ付きで2Dテクスチャを作成
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Width = width_;
	resDesc.Height = height_;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = format_;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	// UAV(CSからの書き込み)を許可するフラグを立てる
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	currentState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		currentState_,
		nullptr,
		IID_PPV_ARGS(resource_.GetAddressOf())
	);

	assert(SUCCEEDED(hr));

	// 描画時にシェーダーから読み込むための SRV を作成(バインドレスヒープに登録)
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = format_;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDescriptorIndex_ = heapManager_->AllocateIndex();
	heapManager_->CreateSRVforTexture2D(srvDescriptorIndex_, resource_.Get(), srvDesc);

	// CSから書き込むための UAV を作成
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.Format = format_;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	uavDescriptorIndex_ = heapManager_->AllocateIndex();
	heapManager_->CreateUAVforTexture2D(uavDescriptorIndex_, resource_.Get(), uavDesc);

	uavCpuHandle_ = heapManager_->GetCpuHandle(uavDescriptorIndex_);
	uavGpuHandle_ = heapManager_->GetGpuHandle(uavDescriptorIndex_);
}

void RWTexture2D::Release() {
	if (resource_) {
		if (heapManager_) {
			if (srvDescriptorIndex_ != heapManager_->kInvalidDescriptorIndex) {
				heapManager_->FreeIndex(srvDescriptorIndex_);
			}
			if (uavDescriptorIndex_ != 0) {
				heapManager_->FreeIndex(uavDescriptorIndex_);
			}
		}
	}

	resource_ = nullptr;
	width_ = 0;
	height_ = 0;
	srvDescriptorIndex_ = 0;
	uavDescriptorIndex_ = 0;
	heapManager_ = nullptr;
}

void RWTexture2D::Transition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES stateAfter) {
	// すでにその状態なら無駄なバリアを出さない
	if (currentState_ == stateAfter) return;

	MyEngine::Utility::TransitionBarrier(
		cmdList,
		resource_.Get(),
		currentState_, // 現在の状態から
		stateAfter     // 目的の状態へ
	);

	// 現在の状態を更新
	currentState_ = stateAfter;
}