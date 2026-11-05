#pragma once

// 前方宣言
namespace MyEngine::LowLevel {
	class DescriptorHeapPoolContext;
}

namespace MyEngine::Rendering {
	struct PSODescriptor;
	class RenderSystem;
	class DepthTexture;

	class ShadowPass {
	public:
		// コンストラクタ
		ShadowPass(
			ID3D12Device* device,
			LowLevel::DescriptorHeapPoolContext* heapContext
		);

		// デストラクタ
		~ShadowPass();

		// ライト方向と中心からライトVP行列を計算
		void Update(const Vector3& lightDir, const Vector3& center);

		// シャドウマップへ深度を描く
		void Execute(ID3D12GraphicsCommandList* cmdList, RenderSystem* renderSystem);

		// 影用PSOの設定をDescに反映する（PSO自体はRendererのSubmitで作る）
		void ApplySettings(PSODescriptor& desc) const;

		// rangeやbiasの調整用
		void ImGui();

		const Matrix4x4& GetLightViewProj() const;
		uint32_t GetShadowMapSrvIndex() const;
		float GetBias() const;

		// 影の中心（プレイヤー位置など）を指定する。毎フレーム呼ぶ
		void SetCenter(const Vector3& center) { center_ = center; hasCenter_ = true; }
		// SetCenter されていればその位置、無ければ fallback を返し、フラグを戻す
		Vector3 ConsumeCenter(const Vector3& fallback) {
			Vector3 result = hasCenter_ ? center_ : fallback;
			hasCenter_ = false;
			return result;
		}

	private:
		Vector3 center_{};
		bool hasCenter_ = false;	// このフレームで SetCenter されたか
		std::unique_ptr<DepthTexture> shadowMap_;
		Matrix4x4 lightViewProj_{};
		float range_ = 80.0f;
		float bias_ = 0.001f;
		D3D12_CULL_MODE cullMode_ = D3D12_CULL_MODE_BACK;	// アクネが酷ければ FRONT も試す
		static constexpr uint32_t kShadowMapSize = 2048;
	};
}
