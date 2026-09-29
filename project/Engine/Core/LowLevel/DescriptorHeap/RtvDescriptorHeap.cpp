#include "PCH.h"
#include "RtvDescriptorHeap.h"

MyEngine::LowLevel::RtvDescriptorHeap::RtvDescriptorHeap(ID3D12Device* device, uint32_t maxDescriptors)
	: BaseDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, maxDescriptors, D3D12_DESCRIPTOR_HEAP_FLAG_NONE) {
}