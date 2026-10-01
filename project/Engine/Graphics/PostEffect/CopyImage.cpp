#include "PCH.h"
#include "CopyImage.h"
#include "RenderTexture.h"

void CopyImageEffect::Initialize(ID3D12Device* /*device*/) {
	shadingModel_ = MyEngine::Rendering::ShadingModel::PostEffect_CopyImage;
}

void CopyImageEffect::ImGui() {
	// 調整するパラメータはないので空
}

D3D12_GPU_VIRTUAL_ADDRESS CopyImageEffect::GetConstantBufferAddress() const {
	return 0;
}
