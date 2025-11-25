#include <cassert>
#include <fstream>
#include "RootSignatureManager.h"
#include "DxCommon.h"
#include "Logger.h"

//static変数の実体定義
//standard3Dの実体の定義
D3D12_ROOT_PARAMETER RootSignatureManager::standard3DRootParameters[4] = {
	// 0番目: CBV (VertexShader)
	{ D3D12_ROOT_PARAMETER_TYPE_CBV, { 0, 0 }, D3D12_SHADER_VISIBILITY_VERTEX },
	// 1番目: CBV (PixelShader)
	{ D3D12_ROOT_PARAMETER_TYPE_CBV, { 1, 0 }, D3D12_SHADER_VISIBILITY_PIXEL },
	// 2番目: DescriptorTable (PixelShader) <- DescriptorRangesへのポインタは CreateRootSigDesc で設定
	{ D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, standard3DDescriptorRanges }, D3D12_SHADER_VISIBILITY_PIXEL },
	// 3番目: 平行光源用のCBV (PixelShader)
	{ D3D12_ROOT_PARAMETER_TYPE_CBV, { 3, 0 }, D3D12_SHADER_VISIBILITY_PIXEL },
};

D3D12_DESCRIPTOR_RANGE RootSignatureManager::standard3DDescriptorRanges[1] = {
	// 0番目: SRV
		{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND }
};

D3D12_STATIC_SAMPLER_DESC RootSignatureManager::standard3DStaticSamplers[1] = {
	{
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f, // MipLODBias
		0,    // MaxAnisotropy
		D3D12_COMPARISON_FUNC_NEVER, // ComparisonFunc
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK, // BorderColor
		0.0f, // MinLOD
		D3D12_FLOAT32_MAX, // MaxLOD
		0, // ShaderRegister
		0, // RegisterSpace
		D3D12_SHADER_VISIBILITY_PIXEL // ShaderVisibility
	}
};

//Particleの実体の定義
D3D12_ROOT_PARAMETER RootSignatureManager::particleRootParameters[4] = {
	// 0番目: CBV (VertexShader)
	{ D3D12_ROOT_PARAMETER_TYPE_SRV, { 0, 0 }, D3D12_SHADER_VISIBILITY_VERTEX },
	// 1番目: CBV (PixelShader)
	{ D3D12_ROOT_PARAMETER_TYPE_CBV, { 1, 0 }, D3D12_SHADER_VISIBILITY_PIXEL },
	// 2番目: DescriptorTable (PixelShader) <- DescriptorRangesへのポインタは CreateRootSigDesc で設定
	{ D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, standard3DDescriptorRanges }, D3D12_SHADER_VISIBILITY_PIXEL },
	// 3番目: 平行光源用のCBV (PixelShader)
	{ D3D12_ROOT_PARAMETER_TYPE_CBV, { 3, 0 }, D3D12_SHADER_VISIBILITY_PIXEL },
};

D3D12_DESCRIPTOR_RANGE RootSignatureManager::particleDescriptorRanges[1] = {
	// 0番目: SRV
		{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND }
};

D3D12_STATIC_SAMPLER_DESC RootSignatureManager::particleStaticSamplers[1] = {
	{
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f, // MipLODBias
		0,    // MaxAnisotropy
		D3D12_COMPARISON_FUNC_NEVER, // ComparisonFunc
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK, // BorderColor
		0.0f, // MinLOD
		D3D12_FLOAT32_MAX, // MaxLOD
		0, // ShaderRegister
		0, // RegisterSpace
		D3D12_SHADER_VISIBILITY_PIXEL // ShaderVisibility
	}
};

void RootSignatureManager::Initialize (DxCommon* dxCommon) {
	device_ = dxCommon->GetDevice ();
}

uint32_t RootSignatureManager::GetOrCreateRootSignature (RootSigType type) {
	std::ofstream os = Logger::Logtext ();
	HRESULT hr;

	//RootSigTypeからdescを作成
	D3D12_ROOT_SIGNATURE_DESC desc = CreateRootSigDesc (type);

	//ハッシュを計算する
	uint64_t hash = ComputeHash (desc);

	//ハッシュをチェック
	if (m_HashTagID.count (hash)) {
		//ヒットしたら既存のIDを返す
		return m_HashTagID.at (hash);
	}

	//ハッシュがヒットしなかったら新規作成
	ComPtr<ID3DBlob> signatureBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	//RootSignatureのシリアライズ(バイナリ変換)
	hr = D3D12SerializeRootSignature (
		&desc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signatureBlob,
		&errorBlob
	);
	//失敗したら
	if (FAILED (hr)) {
		//ログを出力
		Logger::Log (os, reinterpret_cast<char*>(errorBlob->GetBufferPointer ()));
		assert (false && "RootSignature serialization failed!");
		return 0;
	}

	//デバイスでRootSignatureを作成
	ComPtr<ID3D12RootSignature> newRootSig;
	hr = device_->CreateRootSignature (
		0,		//ノードマスク(単一GPUなら0)
		signatureBlob->GetBufferPointer (),
		signatureBlob->GetBufferSize (),
		IID_PPV_ARGS (newRootSig.GetAddressOf ())
	);
	assert (SUCCEEDED (hr) && "Root Signature creation failed!");
	if (FAILED (hr)) return 0;

	//キャッシュに登録
	//IDを割り当てた後インクリメント
	uint32_t newID = m_NextID++;

	m_RootSigCache[newID] = newRootSig;	//実体データの登録
	m_HashTagID[hash] = newID;			//逆引きハッシュの登録

	return newID;
}

ID3D12RootSignature* RootSignatureManager::GetRootSignature (uint32_t rootSigID) const {
	// IDが存在するかチェック
	if (m_RootSigCache.count (rootSigID)) {
		//存在していたらそれを返す
		return m_RootSigCache.at (rootSigID).Get ();
	}
	// 見つからなかったらassertでnullptrを返す
	assert (false && "RootSignature ID not found in cache!");
	return nullptr;
}

uint64_t RootSignatureManager::ComputeHash (const D3D12_ROOT_SIGNATURE_DESC& desc) const {
	//構造体の主要なメンバをハッシュの種
	uint64_t hash = 0;

	//フラグ、数などをハッシュ化
	hash = (hash * 31) ^ desc.Flags;
	hash = (hash * 31) ^ desc.NumParameters;
	hash = (hash * 31) ^ desc.NumStaticSamplers;

	//ルートパラメータの内容をハッシュ化
	for (UINT i = 0; i < desc.NumParameters; ++i) {
		const auto& param = desc.pParameters[i];

		//パラメータタイプやシェーダーの可視性をハッシュ化
		hash = (hash * 31) ^ param.ParameterType;
		hash = (hash * 31) ^ param.ShaderVisibility;

		//パラメータタイプがCBVorSRVorUAVなら
		if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV ||
			param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV ||
			param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV) {

			//Descriptor.ShaderRegisterやDescriptor.Register	Spaceをハッシュ化
			hash = (hash * 31) ^ param.Descriptor.ShaderRegister;
			hash = (hash * 31) ^ param.Descriptor.RegisterSpace;
		}
		//パラメータタイプがDescriptorTableなら
		else if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
			//DescriptorTableの中身をハッシュ化
			const auto& table = param.DescriptorTable;
			hash = (hash * 31) ^ table.NumDescriptorRanges;
			for (UINT j = 0; j < table.NumDescriptorRanges; ++j) {
				const auto& range = table.pDescriptorRanges[j];
				hash = (hash * 31) ^ range.RangeType;
				hash = (hash * 31) ^ range.BaseShaderRegister;
				hash = (hash * 31) ^ range.NumDescriptors;
			}
		}
	}
	//スタティックサンプラーの内容をハッシュ化
	for (UINT i = 0; i < desc.NumStaticSamplers; ++i) {
		const auto& sampler = desc.pStaticSamplers[i];
		//主要な設定をハッシュの種にする
		hash = (hash * 31) ^ sampler.Filter; // フィルタリングモード
		hash = (hash * 31) ^ sampler.AddressU; // U座標のアドレスモード
		hash = (hash * 31) ^ sampler.AddressV; // V座標のアドレスモード
		hash = (hash * 31) ^ sampler.AddressW; // W座標のアドレスモード
		hash = (hash * 31) ^ sampler.ShaderRegister; // シェーダーレジスタ
		hash = (hash * 31) ^ sampler.ShaderVisibility; // 可視性
		hash = (hash * 31) ^ sampler.ComparisonFunc; // 比較関数

		//他の設定も必要に応じてハッシュ化
		hash = (hash * 31) ^ (uint32_t)(sampler.MaxLOD * 100.0f);
	}

	return hash;
}

D3D12_ROOT_SIGNATURE_DESC RootSignatureManager::CreateRootSigDesc (RootSigType type) {
	D3D12_ROOT_SIGNATURE_DESC desc = {};

	//作る設定を分岐させる
	switch (type) {
	case RootSigType::Standard3D:
		//RootSignature
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		//RootParameter作成。複数設定できるので配列
		desc.pParameters = standard3DRootParameters;				//ルートパラメータ配列へのポインタ
		desc.NumParameters = _countof (standard3DRootParameters);	//配列の長さ

		//Sampler
		desc.pStaticSamplers = standard3DStaticSamplers;
		desc.NumStaticSamplers = _countof (standard3DStaticSamplers);

		break;

	case RootSigType::Particle:
		//RootSignature
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		//RootParameter作成。複数設定できるので配列
		desc.pParameters = particleRootParameters;				//ルートパラメータ配列へのポインタ
		desc.NumParameters = _countof (particleRootParameters);	//配列の長さ

		//Sampler
		desc.pStaticSamplers = particleStaticSamplers;
		desc.NumStaticSamplers = _countof (particleStaticSamplers);
		break;
	}

	return desc;
}
