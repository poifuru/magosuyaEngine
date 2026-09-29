#include "PCH.h"
#include "DescriptorHeapPoolContext.h"

void MyEngine::LowLevel::DescriptorHeapPoolContext::Initialize(ID3D12Device* device) {
	assert(device != nullptr);

	dsvPool_ = std::make_unique<DsvDescriptorHeapPool>(device);
	rtvPool_ = std::make_unique<RtvDescriptorHeapPool>(device);
	srvPool_ = std::make_unique<SrvDescriptorHeapPool>(device);
}