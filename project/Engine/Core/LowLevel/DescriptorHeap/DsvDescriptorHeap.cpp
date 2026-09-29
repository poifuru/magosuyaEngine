#include "PCH.h"
#include "DsvDescriptorHeap.h"

MyEngine::LowLevel::DsvDescriptorHeap::DsvDescriptorHeap(ID3D12Device* device, uint32_t maxDescriptors)
	: BaseDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, maxDescriptors, D3D12_DESCRIPTOR_HEAP_FLAG_NONE) {
}