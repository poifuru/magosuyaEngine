#include "PCH.h"
#include "SrvDescriptorHeap.h"

MyEngine::LowLevel::SrvDescriptorHeap::SrvDescriptorHeap() = default;

void MyEngine::LowLevel::SrvDescriptorHeap::Initialize(ID3D12Device* device, uint32_t maxDescriptors) {
	assert(device != nullptr);
	maxDescriptors_ = maxDescriptors;

	// ディスクリプタサイズ(一個当たり何バイト進めばいいのか)を取得
	descriptorSize_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// 巨大なディスクリプタヒープの作成設定
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = maxDescriptors_;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // シェーダーから見えるようにする
	heapDesc.NodeMask = 0;

	HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(heap_.GetAddressOf()));
	assert(SUCCEEDED(hr));
}

uint32_t MyEngine::LowLevel::SrvDescriptorHeap::AllocateIndex() {
	// 返却された空き枠(キュー)があれば、優先的にそこを再利用する
	if(!freeIndices_.empty()) {
		uint32_t index = freeIndices_.front();
		freeIndices_.pop();
		return index;
	}

	// 新しいインデックスを切り出す(上限チェック)
	assert(nextIndex_ < maxDescriptors_ && "ディスクリプタヒープの最大確保数を超えました");
	uint32_t index = nextIndex_;
	nextIndex_++;

	return index;
}

void MyEngine::LowLevel::SrvDescriptorHeap::FreeIndex(uint32_t index) {
	// 使い終わったインデックスを再利用リストに積む
	freeIndices_.push(index);
}

void MyEngine::LowLevel::SrvDescriptorHeap::CreateCBV(uint32_t index, const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc) {
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	HRESULT hr = heap_->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
	assert(SUCCEEDED(hr));
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GetCpuHandle(index);
	device->CreateConstantBufferView(&desc, cpuHandle);
}

void MyEngine::LowLevel::SrvDescriptorHeap::CreateSRVforTexture2D(uint32_t index, ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc) {
	// 瞬間的に生デバイスを取得
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	HRESULT hr = heap_->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
	assert(SUCCEEDED(hr));

	// 自分の対応するCPUハンドルを取得して、そこにSRVを焼き付ける
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GetCpuHandle(index);
	device->CreateShaderResourceView(resource, &desc, cpuHandle);
}

void MyEngine::LowLevel::SrvDescriptorHeap::CreateUAVforTexture2D(uint32_t index, ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc) {
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	HRESULT hr = heap_->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
	assert(SUCCEEDED(hr));
	// インデックス位置のCPUハンドルを取得してUAVを焼き付ける
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GetCpuHandle(index);
	device->CreateUnorderedAccessView(resource, nullptr, &desc, cpuHandle);
}

void MyEngine::LowLevel::SrvDescriptorHeap::SetGraphicsHeap(ID3D12GraphicsCommandList* cmdList) {
	assert(cmdList != nullptr);

	// コマンドリストにこのヒープをセットする(ドローコールより前に一回だけ呼ぶ)
	ID3D12DescriptorHeap* heaps[] = { heap_.Get() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
}

D3D12_CPU_DESCRIPTOR_HANDLE MyEngine::LowLevel::SrvDescriptorHeap::GetCpuHandle(uint32_t index) const {
	assert(index < maxDescriptors_);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = heap_->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(index) * descriptorSize_;
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE MyEngine::LowLevel::SrvDescriptorHeap::GetGpuHandle(uint32_t index) const {
	assert(index < maxDescriptors_);
	D3D12_GPU_DESCRIPTOR_HANDLE handle = heap_->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(index) * descriptorSize_;
	return handle;
}

uint32_t MyEngine::LowLevel::SrvDescriptorHeap::GetIndex(D3D12_CPU_DESCRIPTOR_HANDLE handle) const {
	D3D12_CPU_DESCRIPTOR_HANDLE startHandle = heap_->GetCPUDescriptorHandleForHeapStart();
	return static_cast<uint32_t>((handle.ptr - startHandle.ptr) / descriptorSize_);
}

uint32_t MyEngine::LowLevel::SrvDescriptorHeap::GetIndex(D3D12_GPU_DESCRIPTOR_HANDLE handle) const {
	D3D12_GPU_DESCRIPTOR_HANDLE startHandle = heap_->GetGPUDescriptorHandleForHeapStart();
	return static_cast<uint32_t>((handle.ptr - startHandle.ptr) / descriptorSize_);
}
