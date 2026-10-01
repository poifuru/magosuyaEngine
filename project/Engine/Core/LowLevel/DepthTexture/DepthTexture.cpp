#include "PCH.h"
#include "DepthTexture.h"
#include "Function.h"

MyEngine::Rendering::DepthTexture::DepthTexture(
	Microsoft::WRL::ComPtr<ID3D12Resource> resource,
	uint32_t dsvIndex,
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
	uint32_t srvIndex,
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle
) {
	resource_ = resource;
	dsvIndex_ = dsvIndex;
	dsvHandle_ = dsvHandle;
	srvIndex_ = srvIndex;
	srvGpuHandle_ = srvGpuHandle;
	currentState_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;
}

void MyEngine::Rendering::DepthTexture::ChangeState(
	ID3D12GraphicsCommandList* cmdList,
	D3D12_RESOURCE_STATES newState
) {
	if (currentState_ != newState) {
		MyEngine::Utility::TransitionBarrier(cmdList, resource_.Get(), currentState_, newState);
		currentState_ = newState;
	}
}
