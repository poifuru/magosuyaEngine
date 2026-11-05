	#include "PCH.h"
#include "RootSignatureManager.h"

void MyEngine::Rendering::RootSignatureManager::Initialize (ID3D12Device* device) {
	assert(device != nullptr);
	HRESULT hr = S_OK;

#pragma region 共通ルートシグネチャ
	// === 共通ルートパラメータの設定（7スロット） ===
	D3D12_ROOT_PARAMETER rootParameters[7] = {};

	// Slot0 : オブジェクト個別トランスフォームバッファ (b0, space0) -> Vertex, Pixel両方から見えるようにする
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[0].Descriptor.ShaderRegister = 0;
	rootParameters[0].Descriptor.RegisterSpace = 0;

	// Slot1 : オブジェクトインデックス送信 (b1, space0) -> 32ビット定数として2個の数値を送る
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[1].Constants.ShaderRegister = 1;
	rootParameters[1].Constants.RegisterSpace = 0;
	rootParameters[1].Constants.Num32BitValues = 2; // materialIndex と textureIndex の2つ

	// Slot2 : バインドレスヒープ全体を指すディスクリプタテーブル (t0, space1)
	// レンジの設定
	D3D12_DESCRIPTOR_RANGE range1{};
	range1.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range1.NumDescriptors = UINT_MAX; // ヒープ内のすべてのSRVにアクセスできるように無限（UINT_MAX）にする！
	range1.BaseShaderRegister = 0;
	range1.RegisterSpace = 1; // space1 に配置する
	range1.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // テクスチャはVS,PSで使う
	rootParameters[2].DescriptorTable.pDescriptorRanges = &range1;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;

	// Slot3 : バインドレスヒープ全体を指すディスクリプタテーブル (t0, space2)
	// レンジの設定
	D3D12_DESCRIPTOR_RANGE range2{};
	range2.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range2.NumDescriptors = UINT_MAX; // ヒープ内のすべてのSRVにアクセスできるように無限（UINT_MAX）にする！
	range2.BaseShaderRegister = 0;
	range2.RegisterSpace = 2; // space2 に配置する
	range2.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // テクスチャはVS,PSで使う
	rootParameters[3].DescriptorTable.pDescriptorRanges = &range2;
	rootParameters[3].DescriptorTable.NumDescriptorRanges = 1;

	// Slot4 : フレーム共通カメラ定数バッファ (b2, space0)
	rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[4].Descriptor.ShaderRegister = 2;
	rootParameters[4].Descriptor.RegisterSpace = 0;

	// Slot5 : フレーム共通ライト定数バッファ (b3, space0)
	rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[5].Descriptor.ShaderRegister = 3;
	rootParameters[5].Descriptor.RegisterSpace = 0;

	// Slot6 : オブジェクト固有のカスタム定数バッファ
	rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameters[6].Descriptor.ShaderRegister = 4;
	rootParameters[6].Descriptor.RegisterSpace = 0;

	D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};

	// --- 静的サンプラーの設定（s0, space0） --- //
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].RegisterSpace = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// --- シャドウマップ比較用サンプラーの設定（s1, space0） --- //
	// 「参照値 <= シャドウマップの深度」なら1（光が当たる）を返す。LINEARなので周囲2x2の結果を補間してくれる
	staticSamplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSamplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;	// 範囲外は影なし
	staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	staticSamplers[1].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[1].ShaderRegister = 1;
	staticSamplers[1].RegisterSpace = 0;
	staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// --- ルートシグネチャのビルド --- //
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	desc.pParameters = rootParameters;
	desc.NumParameters = _countof(rootParameters);
	desc.pStaticSamplers = staticSamplers;
	desc.NumStaticSamplers = _countof(staticSamplers);

	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

	hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	assert(SUCCEEDED(hr));

	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(rootSignature_.GetAddressOf()));
	assert(SUCCEEDED(hr));
#pragma endregion

#pragma region CS用ルートシグネチャ
	// CS用共通ルートシグネチャの作成
	// UAV用のレンジ(u0, space0)
	D3D12_DESCRIPTOR_RANGE uavRange{};
	uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	uavRange.NumDescriptors = 1;
	uavRange.BaseShaderRegister = 0; // u0
	uavRange.RegisterSpace = 0;
	uavRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// バインドレスSRV用のレンジ(t0, space2)
	D3D12_DESCRIPTOR_RANGE srvRange{};
	srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange.NumDescriptors = UINT_MAX;
	srvRange.BaseShaderRegister = 0; // t0
	srvRange.RegisterSpace = 2;      // space2
	srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParams[3]{};

	// Slot0 : UAV(u0, space0)
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[0].DescriptorTable.pDescriptorRanges = &uavRange;
	rootParams[0].DescriptorTable.NumDescriptorRanges = 1;

	// Slot1 : CBV(b0, space0)
	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[1].Descriptor.ShaderRegister = 0; // b0
	rootParams[1].Descriptor.RegisterSpace = 0;

	// Slot2 : バインドレスSRV(t0, space2)
	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParams[2].DescriptorTable.pDescriptorRanges = &srvRange;
	rootParams[2].DescriptorTable.NumDescriptorRanges = 1;

	D3D12_ROOT_SIGNATURE_DESC CSdesc{};
	CSdesc.pParameters = rootParams;
	CSdesc.NumParameters = _countof(rootParams);

	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	Microsoft::WRL::ComPtr<ID3DBlob> CSErrorBlob;

	hr = D3D12SerializeRootSignature(&CSdesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &CSErrorBlob);
	assert(SUCCEEDED(hr));

	hr = device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(computeRootSignature_.GetAddressOf()));
	assert(SUCCEEDED(hr));
#pragma endregion
}