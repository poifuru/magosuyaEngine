#pragma once
#include <Windows.h>
#include <Wrl.h>
using namespace Microsoft::WRL;
#include <d3d12.h>
#include <unordered_map>
#include "DxCommon.h"
#include "ShaderManager.h"
#include "RootSignatureManager.h"
#include "BlendModeManager.h"
#include "InputLayoutManager.h"

struct PSODescriptor {
	// --- マネージャーで管理してるID ---
	uint32_t RootSignatureID = 0;
	uint32_t VS_ID = 0;										// ShaderManagerから取得したVertex Shader ID
	uint32_t PS_ID = 0;										// ShaderManagerから取得したPixel Shader ID							// RootSignatureManagerから取得したID
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
	DXGI_FORMAT RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	UINT NumRenderTargets = 1;

	//DepthStencilの設定
	DXGI_FORMAT DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	//Sampling
	UINT SampleCount = 1;
	UINT SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
};

class PSOManager {
public:		//メンバ関数
	static PSOManager* GetInstance () {
		//初めて呼び出されたときに一回だけ初期化
		static PSOManager instance;
		return &instance;
	}

	void Initialize (DxCommon* dxCommon);

	//PSODesctiptorを受けとってID3D12PipelineState*を返す
	ID3D12PipelineState* GetOrCreratePSO (const PSODescriptor& desc);

private:
	//コンストラクタを禁止
	PSOManager () = default;
	// コピーコンストラクタと代入演算子を禁止
	PSOManager (const PSOManager&) = delete;
	PSOManager& operator=(const PSOManager&) = delete;
	PSOManager (PSOManager&&) = delete;
	PSOManager& operator=(PSOManager&&) = delete;

private:	//ヘルパー関数
	template<typename T>
	uint64_t hash_combine_simple (uint64_t h, T val) const;
	uint64_t ComputeHash (const PSODescriptor& desc) const;

private:	//メンバ変数
	// ハッシュ値とPipelineStateの実体データのマップ
	std::unordered_map<uint64_t, ComPtr<ID3D12PipelineState>> m_PSOCache;

	//ポインタを借りる
	DxCommon* dxCommon_ = nullptr;
	ShaderManager* shaderManager_ = nullptr;
	RootSignatureManager* rootSigManager_ = nullptr;
	BlendModeManager* blendModeManager_ = nullptr;
	InputLayoutManager* inputLayoutManager_ = nullptr;
};