#pragma once
#include "MaterialData.h"
#include "StructuredBuffer.h"
#include "RenderingModel.h"

namespace MyEngine::LowLevel {
	class GraphicsDevice;
	class SrvDescriptorHeapPool;
}

namespace MyEngine::Rendering {
	class RootSignatureManager;
	class PSOManager;
	class ShaderManager;
}

namespace MyEngine::Rendering {
	class Material {
	public:
		Material() = default;
		~Material() = default;

		// デバイスやヒープを用いて初期化
		void Initialize(
			MyEngine::LowLevel::GraphicsDevice* device,
			MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager
		);

		void Update();

		// シェーダーやテクスチャの設定
		MyEngine::Rendering::ShadingModel GetShadingModel() const;
		void SetShadingModel(MyEngine::Rendering::ShadingModel shadingModel);

		MyEngine::Rendering::BlendModeType GetBlendMode() const;
		void SetBlendMode(MyEngine::Rendering::BlendModeType blenMode);

		bool IsDepthEnable() const;
		void SetDepthEnable(bool enable);

		bool IsDoubleSided() const;
		void SetDoubleSided(bool doubleSided);

		bool IsDepthWrite() const;
		void SetDepthWrite(bool writeEnable) { depthWriteEnable_ = writeEnable; }

		uint8_t GetLayer() const;
		void SetLayer(uint8_t layer);

		uint32_t GetTextureIndex() const { return textureIndex_; }
		void SetTextureIndex(uint32_t textureIndex) { textureIndex_ = textureIndex; }

		// アクセッサ
		uint32_t GetDescriptorIndex() const { return buffer_ ? buffer_->GetDescriptorIndex() : 0; } // 定数バッファのヒープインデックス

		// マテリアルパラメータの設定 (変更されたらフラグを立てる)
		void SetColor(const Vector4& color) { data_.color = color; isDirty_ = true; }

		// UV個別設定用
		void SetUvScale(const Vector3& scale) { uvTransform_.scale = scale; isDirty_ = true; }
		void SetUvRotate(const Vector3& rotate) { uvTransform_.rotate = rotate; isDirty_ = true; }
		void SetUvTranslate(const Vector3& translate) { uvTransform_.translate = translate; isDirty_ = true; }
		// まとめて設定したい時用
		void SetUvTransform(const EulerTransform& transform) { uvTransform_ = transform; isDirty_ = true; }

		void SetRoughness(float r) { data_.roughness = r; isDirty_ = true; }
		void SetMetallic(float m) { data_.metallic = m; isDirty_ = true; }
		void SetEnvironmentCoefficient(float e) { data_.environmentCoefficient = e; isDirty_ = true; }
		void SetEnableLighting(BOOL flag) { data_.enableLighting = flag; isDirty_ = true; }
		void SetTime(float time) { data_.time = time; isDirty_ = true; }

		// ImGuiでのパラメータ変更用
		const MaterialData& GetMaterialData() const { return data_; }
		const EulerTransform& GetUvTransform() const { return uvTransform_; }

	private:
		MyEngine::Rendering::ShadingModel shadingModel_ = MyEngine::Rendering::ShadingModel::Standard;
		MyEngine::Rendering::BlendModeType blendMode_ = MyEngine::Rendering::BlendModeType::Opaque;

		bool isDepthEnable_ = true;
		bool isDoubleSided_ = true;
		bool depthWriteEnable_ = true;
		uint8_t layer_ = 0;

		uint32_t textureIndex_ = 0;
		
		// GPU側のバッファ (StructuredBufferをマテリアル自身に持たせる)
		std::unique_ptr<StructuredBuffer<MaterialData>> buffer_ = nullptr;
		// CPU側のパラメータ
		MaterialData data_{};
		EulerTransform uvTransform_{};
		// CPUデータが書き換わったかどうかの更新フラグ (無駄な転送を避ける)
		bool isDirty_ = true;
	};
}