#pragma once

namespace MyEngine::Rendering {
	class RootSignatureManager {
	public:		// メンバ関数
		RootSignatureManager() = default;
		~RootSignatureManager() = default;

		void Initialize(ID3D12Device* device);

		// レンダラーやPSOManagerが使うゲッター
		ID3D12RootSignature* GetCommonRootSignature() const { return rootSignature_.Get(); }
		ID3D12RootSignature* GetComputeRootSignature() const { return computeRootSignature_.Get(); }
		ID3D12RootSignature* GetParticleRootSignature() const { return particleRootSignature_.Get(); }

	public:
		// コピー・移動禁止
		RootSignatureManager(const RootSignatureManager&) = delete;
		RootSignatureManager& operator=(const RootSignatureManager&) = delete;
		RootSignatureManager(RootSignatureManager&&) = delete;
		RootSignatureManager& operator=(RootSignatureManager&&) = delete;

	private:	// メンバ変数
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> computeRootSignature_;	// CS用
		Microsoft::WRL::ComPtr<ID3D12RootSignature> particleRootSignature_;	// GpuParticle用
	};
}