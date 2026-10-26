#include "PCH.h"
#include "PSOManager.h"
#include "ShaderManager.h"
#include "BlendModeManager.h"
#include "InputLayoutManager.h"
#include <cassert>
#include "ScopeTimer.h"

ID3D12PipelineState* MyEngine::Rendering::PSOManager::GetOrCreatePSO (
	ID3D12Device* device,
	const MyEngine::Rendering::PSODescriptor& desc,
	ID3D12RootSignature* commonRootSignature,
	const ShaderManager& shaderManager,
	const MyEngine::Rendering::InputLayoutManager& inputLayoutManager,
	const MyEngine::Rendering::BlendModeManager& blendModeManager
) {
	assert(device != nullptr && commonRootSignature != nullptr);

	// 引数からハッシュ計算
	uint64_t hash = ComputeHash(desc);

	// キャッシュを検索
	if (psoCache_.count(hash)) {
		return psoCache_.at(hash).Get();
	}

	// 無ければ新しく作ってキャッシュ登録 //
	// *** 設定する *** //
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

	// RootSignatureを取得(RootSigManagerから)
	psoDesc.pRootSignature = commonRootSignature;

	// Shaderを取得(ShaderManagerから)
	psoDesc.VS = shaderManager.GetShaderBytecode(desc.VS_ID);
	psoDesc.PS = shaderManager.GetShaderBytecode(desc.PS_ID);

	// InputLayoutを取得(InputLayoutManagerから)
	const D3D12_INPUT_LAYOUT_DESC* inputLayoutDesc = inputLayoutManager.GetInputLayout(desc.InputLayoutID);
	psoDesc.InputLayout = *inputLayoutDesc;

	// ブレンドステートを取得(BlendModeManagerから)
	psoDesc.BlendState = blendModeManager.GetBlendDesc(desc.BlendMode);

	// ラスタライザーステート (Descriptorから直接設定)
	D3D12_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.CullMode = desc.CullMode;
	rasterizerDesc.FillMode = desc.FillMode;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.ForcedSampleCount = 0;
	psoDesc.RasterizerState = rasterizerDesc;

	// デプス/ステンシルステート (Descriptorから直接設定)
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = desc.DepthEnable;
	depthStencilDesc.DepthWriteMask = desc.DepthWriteMask;
	depthStencilDesc.DepthFunc = desc.DepthFunc;
	depthStencilDesc.StencilEnable = FALSE; // ステンシルは使わない前提
	psoDesc.DepthStencilState = depthStencilDesc;

	// 出力ターゲットの設定 (Descriptorから直接設定)
	psoDesc.NumRenderTargets = desc.NumRenderTargets;
	psoDesc.RTVFormats[0] = desc.RTVFormat; // 複数のRTが必要ならDescを配列にする
	psoDesc.DSVFormat = desc.DSVFormat;

	// その他
	psoDesc.PrimitiveTopologyType = desc.PrimitiveTopologyType;
	psoDesc.SampleDesc.Count = desc.SampleCount;
	psoDesc.SampleMask = desc.SampleMask;

	// *** 実際にPSOを生成 *** //
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
	HRESULT hr = S_OK;
	{
		// キャッシュに無かった時（新規生成時）だけ計測される
		SCOPE_TIMER(std::format("PSO生成 hash:{:016x}", hash));
		hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(pso.GetAddressOf()));
	}
	assert(SUCCEEDED(hr));

	psoCache_[hash] = pso;
	return pso.Get();
}

// 簡単なハッシュ結合用関数
inline void hash_combine_simple(uint64_t& seed, uint64_t value) {
	seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

uint64_t MyEngine::Rendering::PSOManager::ComputeHash (const PSODescriptor& desc) const {
	uint64_t hash = 0;

	// --- マネージャー管理のIDをハッシュ化（最重要）---
	// IDが変われば、シェーダーやルートシグネチャも変わるのでハッシュは必ず変える
	hash_combine_simple (hash, desc.VS_ID);
	hash_combine_simple (hash, desc.PS_ID);
	// InputLayoutIDの型をInputLayoutType(enum class)に合わせた場合も、uint32_tに変換してOK
	hash_combine_simple (hash, (uint32_t)desc.InputLayoutID);
	// BlendModeType (enum class)は、ManagerでD3D12_BLEND_DESCに変換されるので、IDをハッシュ化する
	hash_combine_simple (hash, (uint32_t)desc.BlendMode);

	// --- ラスタライザーステート ---
	hash_combine_simple (hash, (uint32_t)desc.CullMode);
	hash_combine_simple (hash, (uint32_t)desc.FillMode);

	// --- デプス/ステンシルステート ---
	// BOOLはTRUE(1)かFALSE(0)なのでそのままハッシュ化できる
	hash_combine_simple (hash, desc.DepthEnable);
	hash_combine_simple (hash, (uint32_t)desc.DepthWriteMask);
	hash_combine_simple (hash, (uint32_t)desc.DepthFunc);

	// --- その他の設定 ---
	hash_combine_simple (hash, (uint32_t)desc.PrimitiveTopologyType);
	hash_combine_simple (hash, (uint32_t)desc.RTVFormat);
	hash_combine_simple (hash, desc.NumRenderTargets);
	hash_combine_simple (hash, (uint32_t)desc.DSVFormat);

	// --- サンプリング ---
	hash_combine_simple (hash, desc.SampleCount);
	// SampleMaskは32bit値なので、uint32_tでキャストするだけで十分
	hash_combine_simple (hash, desc.SampleMask);

	return hash;
}