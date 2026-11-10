#pragma once
#include "BasePostEffect.h"
#include "ConstantBuffer.h"

class Fog : public BasePostEffect {
public:
	void Initialize(ID3D12Device* device) override;

	void ImGui() override;

	const char* GetDisplayName() const override;

	D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferAddress() const override;

	uint32_t GetSrvIndexSceneColor() { return srvIndexSceneColor_; }
	uint32_t GetSrvIndexDepth() { return srvIndexDepth_; }

private:
	struct alignas(16) FogForGPU {
		Matrix4x4 inverseVP;
		Vector4 color;
		float start;
		float end;
		float cameraNear;
		float cameraFar;
		float heightStart;
		float heightEnd;
		float padding1[2];
		Vector3 cameraPosition;
		float padding2;
	};

private:
	std::unique_ptr<ConstantBuffer<FogForGPU>> buffer_;
	FogForGPU param_;

	uint32_t srvIndexSceneColor_ = 0; // シーンカラー用のインデックス
	uint32_t srvIndexDepth_ = 0;      // デプスバッファ用のインデックス
};