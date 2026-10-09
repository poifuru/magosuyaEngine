#pragma once
#include "TransformMatrixData.h"
#include "Material.h"

namespace MyEngine::Rendering {
	class Renderer;
	class Material;
}

struct CameraData;

class BaseObject3d {
public:
	BaseObject3d();
	virtual ~BaseObject3d() = default;

	void Initialize();

	virtual void Update(const CameraData* cameraData);

	virtual void ImGui(const std::string& label);

	// === 共通のアクセッサ ===
	void SetParentTransform(const EulerTransform* parentTransform) { parentTransform_ = parentTransform; }
	void SetScale(const Vector3& scale) { transform_.scale = scale; }
	void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }
	void SetTranslate(const Vector3& translate) { transform_.translate = translate; }

	// マテリアルをセットする
	void SetMaterial(const std::shared_ptr<MyEngine::Rendering::Material>& material);
	std::shared_ptr<MyEngine::Rendering::Material> GetMaterial() const { return material_; }

	// デプスの有効・無効を設定するセッター
	void SetDepthEnable(bool flag);
	bool GetDepthEnable() const { return isDepthEnable_; }

	// ブレンドモードを設定するゲッター・セッター
	void SetBlendMode(MyEngine::Rendering::BlendModeType mode);
	MyEngine::Rendering::BlendModeType GetBlendMode() const { return blendMode_; }

	// 両面表示を設定するセッター
	void SetDoubleSided(bool flag);
	bool GetDoubleSided() const { return isDoubleSided_; }

	// 外部（ファクトリ）から初期化済みの共通バッファを受け取る
	void SetTransformBuffer(std::unique_ptr<TransformMatrixResource>&& transformBuffer) {
		transformBuffer_ = std::move(transformBuffer);
	}

	void SetLayer(uint8_t layer);
	uint8_t GetLayer() const;

	D3D12_GPU_VIRTUAL_ADDRESS GetTransformGPUAddress() const { return transformBuffer_ ? transformBuffer_->GetGPUVirtualAddress() : 0; }
	uint32_t GetMaterialDescriptorIndex() const { return material_ ? material_->GetDescriptorIndex() : 0; } // バインドレス用インデックスゲッター

protected:
	virtual Matrix4x4 CalculateWorldMatrix();

protected:
	static inline uint32_t instanceID_ = 0;

	// CPUデータ
	EulerTransform transform_{};
	const EulerTransform* parentTransform_ = nullptr;
	TransformMatrixData transformMatrixData_{};

	std::unique_ptr<TransformMatrixResource> transformBuffer_ = nullptr;
	std::shared_ptr<MyEngine::Rendering::Material> material_ = nullptr;

	MyEngine::Rendering::BlendModeType blendMode_ = MyEngine::Rendering::BlendModeType::Opaque; // デフォルトは不透明

	bool isDepthEnable_ = true; // デフォルトはデプス有効
	bool isDoubleSided_ = true; // デフォルトは両面表示

	uint8_t layer_ = 0;
};
