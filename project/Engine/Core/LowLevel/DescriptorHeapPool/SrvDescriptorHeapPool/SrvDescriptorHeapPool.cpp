#include "PCH.h"
#include "SrvDescriptorHeapPool.h"

MyEngine::LowLevel::SrvDescriptorHeapPool::SrvDescriptorHeapPool(ID3D12Device* device, uint32_t maxDescriptors)
	: BaseDescriptorHeapPool(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, maxDescriptors, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) {
}

void MyEngine::LowLevel::SrvDescriptorHeapPool::CreateCBV(uint32_t index, const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc) {
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	HRESULT hr = heap_->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
	assert(SUCCEEDED(hr));
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GetCpuHandle(index);
	device->CreateConstantBufferView(&desc, cpuHandle);
}

void MyEngine::LowLevel::SrvDescriptorHeapPool::CreateSRVforTexture2D(uint32_t index, ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc) {
	// 瞬間的に生デバイスを取得
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	HRESULT hr = heap_->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
	assert(SUCCEEDED(hr));

	// 自分の対応するCPUハンドルを取得して、そこにSRVを焼き付ける
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GetCpuHandle(index);
	device->CreateShaderResourceView(resource, &desc, cpuHandle);
}

void MyEngine::LowLevel::SrvDescriptorHeapPool::CreateUAVforTexture2D(uint32_t index, ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc) {
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	HRESULT hr = heap_->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
	assert(SUCCEEDED(hr));
	// インデックス位置のCPUハンドルを取得してUAVを焼き付ける
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = GetCpuHandle(index);
	device->CreateUnorderedAccessView(resource, nullptr, &desc, cpuHandle);
}

void MyEngine::LowLevel::SrvDescriptorHeapPool::SetGraphicsHeap(ID3D12GraphicsCommandList* cmdList) {
	assert(cmdList != nullptr);

	// コマンドリストにこのヒープをセットする(ドローコールより前に一回だけ呼ぶ)
	ID3D12DescriptorHeap* heaps[] = { heap_.Get() };
	cmdList->SetDescriptorHeaps(_countof(heaps), heaps);
}

D3D12_GPU_DESCRIPTOR_HANDLE MyEngine::LowLevel::SrvDescriptorHeapPool::GetGpuHandle(uint32_t index) const {
	assert(index < maxDescriptors_);
	D3D12_GPU_DESCRIPTOR_HANDLE handle = heap_->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(index) * descriptorSize_;
	return handle;
}

uint32_t MyEngine::LowLevel::SrvDescriptorHeapPool::GetIndex(D3D12_GPU_DESCRIPTOR_HANDLE handle) const {
	D3D12_GPU_DESCRIPTOR_HANDLE startHandle = heap_->GetGPUDescriptorHandleForHeapStart();
	return static_cast<uint32_t>((handle.ptr - startHandle.ptr) / descriptorSize_);
}
