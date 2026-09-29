#pragma once
#include "ConstantBuffer.h"
#include "RWTexture2D.h"
#include "ShaderManager.h"
#include "SrvDescriptorHeapPool.h"
#include "Function.h"

template <typename T>
class ComputePipeline {
public:
	ComputePipeline() = default;
	~ComputePipeline() = default;

	void Initialize(
		ID3D12Device* device,
		MyEngine::LowLevel::SrvDescriptorHeapPool& heapManager,
		MyEngine::Rendering::ShaderManager& shaderManager,
		const std::wstring& shaderPath,
		ID3D12RootSignature* computeRootSignature,
		uint32_t width = 512,
		uint32_t height = 512,
		DXGI_FORMAT format = DXGI_FORMAT_R16G16B16A16_FLOAT
	) {
		assert(device != nullptr);

		heapManager_ = &heapManager;

		width_ = width;
		height_ = height;

		// 書き込み用 2D テクスチャの初期化
		outputTexture_.Initialize(device, heapManager, width_, height_, format);

		// 定数バッファ<T> の初期化
		paramBuffer_.Initialize(device);

		// CS用のルートシグネチャを保存
		computeRootSig_ = computeRootSignature;

		// 指定された CS ファイルをコンパイルして Compute PSO を作成
		uint32_t csID = shaderManager.CompileAndCacheShader(shaderPath, L"cs_6_0");
		D3D12_SHADER_BYTECODE csBytecode = shaderManager.GetShaderBytecode(csID);
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = computeRootSig_;
		psoDesc.CS = csBytecode;
		HRESULT hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(computePSO_.GetAddressOf()));
		assert(SUCCEEDED(hr));
	}

	void Dispatch(
		ID3D12GraphicsCommandList* cmdList,
		const T& param
	) {
		// 定数バッファの更新
		paramBuffer_.Update(param);

		// テクスチャを CS 書き込み可能状態 (UNORDERED_ACCESS) に遷移
		outputTexture_.Transition(cmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// CS ルートシグネチャと PSO をセット
		cmdList->SetComputeRootSignature(computeRootSig_);
		cmdList->SetPipelineState(computePSO_.Get());

		// パラメータのセット
		// Slot 0: UAV (u0)
		cmdList->SetComputeRootDescriptorTable(0, outputTexture_.GetUavGpuHandle());

		// Slot 1: CBV (b0)
		cmdList->SetComputeRootConstantBufferView(1, paramBuffer_.GetGPUVirtualAddress());

		// Slot 2: バインドレス SRV テーブル (t0, space2)
		cmdList->SetComputeRootDescriptorTable(2, heapManager_->GetGpuHandle(0));

		// スレッドグループ数の計算と Dispatch 発行
		uint32_t groupX = (width_ + 7) / 8;
		uint32_t groupY = (height_ + 7) / 8;
		cmdList->Dispatch(groupX, groupY, 1);

		// 描画シェーダーで読めるように SRV 状態へ自動遷移！
		outputTexture_.Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	}

	void Transition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES state) {
		outputTexture_.Transition(cmdList, state);
	}

	// 作成されたテクスチャのバインドレス SRV インデックスを取得
	uint32_t GetTextureSrvIndex() const { return outputTexture_.GetSrvIndex(); }

private:
	RWTexture2D outputTexture_;
	ConstantBuffer<T> paramBuffer_;

	ID3D12RootSignature* computeRootSig_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> computePSO_;

	uint32_t width_ = 512;
	uint32_t height_ = 512;

	MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager_ = nullptr;
};