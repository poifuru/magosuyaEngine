#include "PSOManager.h"

void PSOManager::Initialize (DxCommon* dxCommon) {
	dxCommon_ = dxCommon;
	commandList_ = dxCommon->GetCommandList ();
	shaderManager_ = ShaderManager::GetInstance ();
	shaderManager_->Initialize (dxCommon);
	rootSigManager_ = RootSignatureManager::GetInstance ();
	rootSigManager_->Initialize (dxCommon);
	blendModeManager_ = BlendModeManager::GetInstance ();
	blendModeManager_->Initialize ();
	inputLayoutManager_ = InputLayoutManager::GetInstance ();
	inputLayoutManager_->Initialize ();
}

ID3D12PipelineState* PSOManager::GetOrCreratePSO (const PSODescriptor& desc) {
	HRESULT hr;

	//引数からハッシュ計算
	uint64_t hash = ComputeHash (desc);

	//キャッシュを検索
	if (m_PSOCache.count (hash)) {
		//ヒットしたらそのままそれを返す
		return m_PSOCache.at (hash).Get ();
	}

	//===無ければ新しく作ってキャッシュ登録===//
	//***設定する***//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPieplineStateDesc = {};

	//RootSignatureを取得(RootSigManagerから)
	graphicsPieplineStateDesc.pRootSignature = rootSigManager_->GetRootSignature (desc.RootSignatureID);

	//Shaderを取得(ShaderManagerから)
	graphicsPieplineStateDesc.VS = shaderManager_->GetShaderBytecode (desc.VS_ID);
	graphicsPieplineStateDesc.PS = shaderManager_->GetShaderBytecode (desc.PS_ID);
	assert (graphicsPieplineStateDesc.VS.BytecodeLength > 0 && "VS bytecode is empty!");
	assert (graphicsPieplineStateDesc.PS.BytecodeLength > 0 && "PS bytecode is empty!");

	//InputLayoutを取得(InputLayoutManagerから)
	const D3D12_INPUT_LAYOUT_DESC* inputLayoutDesc = inputLayoutManager_->GetInputLayout (desc.InputLayoutID);
	graphicsPieplineStateDesc.InputLayout = *inputLayoutDesc;

	//ブレンドステートを取得(BlendModeManagerから)
	graphicsPieplineStateDesc.BlendState = blendModeManager_->GetBlendDesc (desc.BlendMode);

	//ラスタライザーステート (Descriptorから直接設定)
	D3D12_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.CullMode = desc.CullMode;
	rasterizerDesc.FillMode = desc.FillMode;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.ForcedSampleCount = 0;
	graphicsPieplineStateDesc.RasterizerState = rasterizerDesc;

	//デプス/ステンシルステート (Descriptorから直接設定)
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = desc.DepthEnable;
	depthStencilDesc.DepthWriteMask = desc.DepthWriteMask;
	depthStencilDesc.DepthFunc = desc.DepthFunc;
	depthStencilDesc.StencilEnable = FALSE; // ステンシルは使わない前提
	graphicsPieplineStateDesc.DepthStencilState = depthStencilDesc;

	//出力ターゲットの設定 (Descriptorから直接設定)
	graphicsPieplineStateDesc.NumRenderTargets = desc.NumRenderTargets;
	graphicsPieplineStateDesc.RTVFormats[0] = desc.RTVFormat; // 複数のRTが必要ならDescを配列にする
	graphicsPieplineStateDesc.DSVFormat = desc.DSVFormat;

	//その他
	graphicsPieplineStateDesc.PrimitiveTopologyType = desc.PrimitiveTopologyType;
	graphicsPieplineStateDesc.SampleDesc.Count = desc.SampleCount;
	graphicsPieplineStateDesc.SampleMask = desc.SampleMask;
	//******//

	//***実際にPSOを生成***//
	ComPtr<ID3D12PipelineState> newPSO = nullptr;
	ID3D12Device* device = dxCommon_->GetDevice ();
	assert (device != nullptr && "D3D12Device is null!");

	hr = device->CreateGraphicsPipelineState (
		&graphicsPieplineStateDesc,
		IID_PPV_ARGS (newPSO.GetAddressOf ())
	);

	if (FAILED (hr)) {
		assert (false && "CreateGraphicsPipelineState failed!");
		return nullptr;
	}

	//キャッシュに登録
	m_PSOCache[hash] = newPSO;

	//生成したPSOを返す
	return newPSO.Get ();
	//******//
	//======//
}

void PSOManager::SetPSO (const PSODescriptor& desc) {
	auto pso = GetOrCreratePSO (desc);
	commandList_->SetPipelineState (pso);
}

template<typename T>
uint64_t PSOManager::hash_combine_simple (uint64_t h, T val) const {
	return (h << 5) | (h >> 59) ^ (uint64_t)val;
}

uint64_t PSOManager::ComputeHash (const PSODescriptor& desc) const {
	uint64_t hash = 0;

	// --- マネージャー管理のIDをハッシュ化（最重要）---
	// IDが変われば、シェーダーやルートシグネチャも変わるのでハッシュは必ず変えるでやんす
	hash = hash_combine_simple (hash, desc.VS_ID);
	hash = hash_combine_simple (hash, desc.PS_ID);
	hash = hash_combine_simple (hash, desc.RootSignatureID);
	// InputLayoutIDの型をInputLayoutType(enum class)に合わせた場合も、uint32_tに変換してOKでやんす
	hash = hash_combine_simple (hash, (uint32_t)desc.InputLayoutID);
	// BlendModeType (enum class)は、ManagerでD3D12_BLEND_DESCに変換されるので、IDをハッシュ化するでやんす
	hash = hash_combine_simple (hash, (uint32_t)desc.BlendMode);

	// --- ラスタライザーステート ---
	hash = hash_combine_simple (hash, (uint32_t)desc.CullMode);
	hash = hash_combine_simple (hash, (uint32_t)desc.FillMode);

	// --- デプス/ステンシルステート ---
	// BOOLはTRUE(1)かFALSE(0)なのでそのままハッシュ化できるでやんす
	hash = hash_combine_simple (hash, desc.DepthEnable);
	hash = hash_combine_simple (hash, (uint32_t)desc.DepthWriteMask);
	hash = hash_combine_simple (hash, (uint32_t)desc.DepthFunc);

	// --- その他の設定 ---
	hash = hash_combine_simple (hash, (uint32_t)desc.PrimitiveTopologyType);
	hash = hash_combine_simple (hash, (uint32_t)desc.RTVFormat);
	hash = hash_combine_simple (hash, desc.NumRenderTargets);
	hash = hash_combine_simple (hash, (uint32_t)desc.DSVFormat);

	// --- サンプリング ---
	hash = hash_combine_simple (hash, desc.SampleCount);
	// SampleMaskは32bit値なので、uint32_tでキャストするだけで十分でやんす
	hash = hash_combine_simple (hash, desc.SampleMask);

	return hash;
}