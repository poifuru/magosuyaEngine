#include "PCH.h"
#include "DsvDescriptorHeapPool.h"

MyEngine::LowLevel::DsvDescriptorHeapPool::DsvDescriptorHeapPool(ID3D12Device* device, uint32_t maxDescriptors)
	: BaseDescriptorHeapPool(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, maxDescriptors, D3D12_DESCRIPTOR_HEAP_FLAG_NONE) {
}