#include "PCH.h"
#include "Material.h"
#include "GraphicsDevice.h"
#include "SrvDescriptorHeap.h"
#include "MathFunction.h"
#include "ColorUtils.h"

void MyEngine::Rendering::Material::Initialize(
	MyEngine::LowLevel::GraphicsDevice* device,
	MyEngine::LowLevel::SrvDescriptorHeap* heapManager
) {
	buffer_ = std::make_unique<StructuredBuffer<MaterialData>>();
	buffer_->Initialize(device->GetDevice(), *heapManager, 1);

	// デフォルト値の設定
	data_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	data_.uvTransform = Math::MakeIdentity4x4();
	data_.roughness = 0.5f;
	data_.metallic = 0.0f;
	data_.environmentCoefficient = 0.3f;
	data_.enableLighting = TRUE;

	uvTransform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

	isDirty_ = true;
}

void MyEngine::Rendering::Material::Update() {
	// 値が書き換わっている時だけGPUに送る
	if(isDirty_ && buffer_) {
		// 送信用の一時データを作る
		MaterialData sendData = data_;

		// 行列を計算して詰める
		sendData.uvTransform = Math::MakeAffineMatrix(
			uvTransform_.scale, uvTransform_.rotate, uvTransform_.translate
		);

		// Linearに変換
		sendData.color = ColorUtils::ToLinear(data_.color);

		buffer_->Update({ sendData });
		isDirty_ = false;
	}
}

MyEngine::Rendering::ShadingModel MyEngine::Rendering::Material::GetShadingModel() {
	return shadingModel_;
}

void MyEngine::Rendering::Material::SetShadingModel(MyEngine::Rendering::ShadingModel shadingModel) {
	shadingModel_ = shadingModel;
}

MyEngine::Rendering::BlendModeType MyEngine::Rendering::Material::GetBlendMode() {
	return blendMode_;
}

void MyEngine::Rendering::Material::SetBlendMode(MyEngine::Rendering::BlendModeType blendMode) {
	blendMode_ = blendMode;
	// ブレンドモードに合わせてデフォルトのlayerを自動設定
	if (blendMode == BlendModeType::Opaque) {
		layer_ = 0;
	} else {
		layer_ = 1; // 半透明系は全部1
	}
}

bool MyEngine::Rendering::Material::IsDepthEnable() {
	return isDepthEnable_;
}

void MyEngine::Rendering::Material::SetDepthEnable(bool enable) {
	isDepthEnable_ = enable;
}

bool MyEngine::Rendering::Material::IsDoubleSided() {
	return isDoubleSided_;
}

void MyEngine::Rendering::Material::SetDoubleSided(bool doubleSided) {
	isDoubleSided_ = doubleSided;
}

bool MyEngine::Rendering::Material::IsDepthWrite() {
	return depthWriteEnable_;
}

uint8_t MyEngine::Rendering::Material::GetLayer() {
	return layer_;
}

void MyEngine::Rendering::Material::SetLayer(uint8_t layer) {
	layer_ = layer;
}