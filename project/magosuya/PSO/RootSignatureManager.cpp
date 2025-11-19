#include <cassert>
#include <fstream>
#include "RootSignatureManager.h"
#include "DxCommon.h"
#include "Logger.h"

RootSignatureManager::RootSignatureManager (DxCommon* dxCommon) {
	device_ = dxCommon->GetDevice ();
}

RootSignatureManager::~RootSignatureManager () {

}

uint32_t RootSignatureManager::GetOrCreateRootSignature (const D3D12_ROOT_SIGNATURE_DESC& desc) {
	std::ofstream os = Logger::Logtext ();
	HRESULT hr;

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
		return m_RootSigCache.at (rootSigID).Get();
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
				const auto& range = table.pDescriptorRanges[i];
				hash = (hash * 31) ^ range.RangeType;
				hash = (hash * 31) ^ range.BaseShaderRegister;
				hash = (hash * 31) ^ range.NumDescriptors;
			}
		}
	}
	//スタティックサンプラーの内容をハッシュ化(今回は省略)
	/*for (UINT i = 0; i < desc.NumStaticSamplers; ++i) {

	}*/

	return hash;
}
