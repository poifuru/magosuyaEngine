#include "RootSignatureManager.h"
#include "DxCommon.h"

RootSignatureManager::RootSignatureManager (DxCommon* dxCommon) {
	device_ = dxCommon->GetDevice ();
}

RootSignatureManager::~RootSignatureManager () {

}

uint32_t RootSignatureManager::GetOrCreateRootSignature (const D3D12_ROOT_SIGNATURE_DESC& descriptor) {
	return 0;
}

ID3D12RootSignature* RootSignatureManager::GetRootSignature (uint32_t rootSigID) const {
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
	}

	return 0;
}
