#pragma once
#include "BlendModeManager.h"
#include "InputLayoutManager.h"

// 前方宣言
namespace MyEngine::Rendering {
	class ShaderManager;
	class RootSignature;

	struct PSODescriptor {
		// --- マネージャーで管理してるID ---
		uint32_t VS_ID = 0;										// ShaderManagerから取得したVertex Shader ID
		uint32_t PS_ID = 0;										// ShaderManagerから取得したPixel Shader ID
		bool UsePixelShader = true;								// PixelShaderを使うかのフラグ
		InputLayoutType InputLayoutID = (InputLayoutType)0;		// InputLayoutManagerから取得したID
		BlendModeType BlendMode = BlendModeType::Opaque;		// BlendModeManagerから取得する

		//===保持する設定===//
		//ラスタライザステート
		D3D12_CULL_MODE CullMode = D3D12_CULL_MODE_NONE;	//両面表示
		D3D12_FILL_MODE FillMode = D3D12_FILL_MODE_SOLID;	//塗りつぶし

		//デプスステンシルステート
		BOOL DepthEnable = TRUE;
		D3D12_DEPTH_WRITE_MASK DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		D3D12_COMPARISON_FUNC DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		//===その他の設定===//
		//書き込むRTVの情報
		//トポロジータイプ
		D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		//RTVの情報
		UINT NumRenderTargets = 1;
		DXGI_FORMAT RTVFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

		//DepthStencilの設定
		DXGI_FORMAT DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		//Sampling
		UINT SampleCount = 1;
		UINT SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

		// シャドウマップ用の設定
		UINT DepthBias = 0;
		float SlopeScaledDepthBias = 0.0f;
	};

	class PSOManager {
	public:		//メンバ関数
		PSOManager() = default;
		~PSOManager() = default;

		//PSODescriptorを受けとってID3D12PipelineState*を返す
		ID3D12PipelineState* GetOrCreatePSO(
			ID3D12Device* device,
			const PSODescriptor& desc,
			ID3D12RootSignature* commonRootSignature,
			const ShaderManager& shaderManager,
			const MyEngine::Rendering::InputLayoutManager& inputLayoutManager,
			const MyEngine::Rendering::BlendModeManager& blendModeManager
		);

	public:
		// コピー・移動禁止
		PSOManager(const PSOManager&) = delete;
		PSOManager& operator=(const PSOManager&) = delete;
		PSOManager(PSOManager&&) = delete;
		PSOManager& operator=(PSOManager&&) = delete;

	private:	//ヘルパー関数
		uint64_t ComputeHash(const PSODescriptor& desc) const;

	private:	//メンバ変数
		// ハッシュ値とPipelineStateの実体データのマップ
		std::unordered_map<uint64_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>> psoCache_;
	};
}