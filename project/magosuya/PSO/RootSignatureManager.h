#pragma once
#include <Windows.h>
#include <Wrl.h>
using namespace Microsoft::WRL;
#include <d3d12.h>
#include <string>
#include <unordered_map>

class DxCommon;

class RootSignatureManager {
public:		// メンバ関数
	static RootSignatureManager* GetInstance () {
		//初めて呼び出されたときに一回だけ初期化
		static RootSignatureManager instance;
		return &instance;
	}

	void Initialize (DxCommon* dxCommon);

	//ルートシグネチャの定義を受け取って、生成&キャッシュしてIDを返す
	//(後で定義情報を含む独自のDescriptor構造体を作って差し替え)
	uint32_t GetOrCreateRootSignature (const D3D12_ROOT_SIGNATURE_DESC& desc);

	//IDをもとにID3D12RootSignature*を返す
	ID3D12RootSignature* GetRootSignature (uint32_t rootSigID) const;

private:
	//コンストラクタを禁止
	RootSignatureManager () = default;
	// コピーコンストラクタと代入演算子を禁止
	RootSignatureManager (const RootSignatureManager&) = delete;
	RootSignatureManager& operator=(const RootSignatureManager&) = delete;
	RootSignatureManager (RootSignatureManager&&) = delete;
	RootSignatureManager& operator=(RootSignatureManager&&) = delete;

private:	// ヘルパー関数
	uint64_t ComputeHash (const D3D12_ROOT_SIGNATURE_DESC& desc) const;

private:	// メンバ変数
	//ハッシュ値とIDのマップ(逆引き兼キャッシュチェック用)
	std::unordered_map<uint64_t, uint32_t> m_HashTagID;

	//IDとID3D12RootSignatureの実体データのマップ
	std::unordered_map<uint32_t, ComPtr<ID3D12RootSignature>> m_RootSigCache;

	//次に割り当てるID(0から連番)
	uint32_t m_NextID = 0;

	//ポインタを借りる
	ID3D12Device* device_ = nullptr;
};