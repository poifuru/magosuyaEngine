#pragma once
#include <d3d12.h>
#include <Wrl.h>
using namespace Microsoft::WRL;
#include <unordered_map>

class RootSignatureManager {
public:


private:
	//ハッシュ値とIDのマップ(逆引き兼キャッシュチェック用)
	std::unordered_map<uint64_t, uint32_t> m_HashTagID;

	//IDとID3D12RootSignatureの実体データのマップ
	std::unordered_map<uint32_t, ComPtr<ID3D12RootSignature>> m_RootSigCache;

	//次に割り当てるID(0から連番)
	uint32_t m_NextID = 0;

	//ポインタを借りる
	ID3D12Device* device_ = nullptr;
};