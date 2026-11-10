#pragma once
#include "BasePostEffect.h"
#include "ConstantBuffer.h"

class Outline : public BasePostEffect {
public:
	void Initialize(ID3D12Device* device) override;

	void ImGui() override;

	const char* GetDisplayName() const override;

	void UpdateCameraNearFar(float nearClip, float farClip);

	D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferAddress() const override;


	uint32_t GetSrvIndexSceneColor() { return srvIndexSceneColor_; }
	uint32_t GetSrvIndexDepth() { return srvIndexDepth_; }

private:
	struct alignas(16) OutlineForGPU {
		Vector4 color = { 0.0f, 0.0f, 0.0f, 1.0f }; // 初期値：黒色不透明
		float edgeThreshold = 0.1f;                 // 初期値：感度
		float edgeThickness = 0.5f;                  // 初期値：太さ1倍
		float cameraNear = 0.1f;
		float cameraFar = 1000.0f;
		float padding[3];
	};

private:
	std::unique_ptr<ConstantBuffer<OutlineForGPU>> buffer_;
	OutlineForGPU param_;

	uint32_t srvIndexSceneColor_ = 0; // シーンカラー用のインデックス
	uint32_t srvIndexDepth_ = 0;      // デプスバッファ用のインデックス
};

