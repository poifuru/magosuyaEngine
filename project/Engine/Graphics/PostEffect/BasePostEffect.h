#pragma once
#include "RenderingModel.h"

class BasePostEffect {
public:
	virtual ~BasePostEffect() = default;

	// 継承先でPixelShaderを上書きする
	virtual void Initialize(ID3D12Device* device) = 0;

	// ImGui用関数
	virtual void ImGui() = 0;

	bool GetIsActive() const { return isActive_; }
	void SetIsActive(bool flag) { isActive_ = flag; }

	MyEngine::Rendering::ShadingModel GetShadingModel() const { return shadingModel_; }

	// 描画に必要なデータを渡すための関数
	virtual D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferAddress() const = 0;
	virtual uint32_t GetExtraSrvIndex() const { return 0; } // （追加テクスチャが無ければ0）

protected:
	// ShadingModel
	MyEngine::Rendering::ShadingModel shadingModel_ = MyEngine::Rendering::ShadingModel::PostEffect_CopyImage;

	// ポストエフェクトを有効にするかのフラグ
	bool isActive_ = false;
};