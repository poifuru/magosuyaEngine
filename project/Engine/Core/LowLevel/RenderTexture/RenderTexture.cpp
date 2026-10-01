#include "PCH.h"
#include "RenderTexture.h"
#include "SrvDescriptorHeapPool.h"
#include "Function.h"
#include "WindowsAPI.h"

MyEngine::Rendering::RenderTexture::RenderTexture(
	Microsoft::WRL::ComPtr<ID3D12Resource> resource,
	uint32_t rtvIndex,
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
	uint32_t srvIndex,
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle
) {
	resource_ = resource;
	rtvIndex_ = rtvIndex;
	rtvHandle_ = rtvHandle;
	srvIndex_ = srvIndex;
	srvGpuHandle_ = srvGpuHandle;
	currentState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
}

void MyEngine::Rendering::RenderTexture::ChangeState(
	ID3D12GraphicsCommandList* cmdList,
	D3D12_RESOURCE_STATES newState
) {
		if (currentState_ != newState) {
			MyEngine::Utility::TransitionBarrier(cmdList, resource_.Get(), currentState_, newState);
			currentState_ = newState; // 現在の状態を更新！
		}
}