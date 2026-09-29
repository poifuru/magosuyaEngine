#include "PCH.h"
#include "RtvDescriptorHeapPool.h"

MyEngine::LowLevel::RtvDescriptorHeapPool::RtvDescriptorHeapPool(ID3D12Device* device, uint32_t maxDescriptors)
	: BaseDescriptorHeapPool(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, maxDescriptors, D3D12_DESCRIPTOR_HEAP_FLAG_NONE) {
}