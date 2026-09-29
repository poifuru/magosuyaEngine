#pragma once
#include "BaseDescriptorHeapPool.h"

namespace MyEngine::LowLevel {
	class SrvDescriptorHeapPool : public BaseDescriptorHeapPool {
	public:
		// コンストラクタ
		SrvDescriptorHeapPool(ID3D12Device* device, uint32_t maxDescriptors = 4096);

		// デストラクタ
		~SrvDescriptorHeapPool() override = default;

		// 定数バッファ作成関数
		void CreateCBV(uint32_t index, const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc);

		// ビュー(SRV)生成の委譲窓口
		void CreateSRVforTexture2D(uint32_t index, ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);

		// UAV作成関数
		void CreateUAVforTexture2D(uint32_t index, ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc);

		// 描画パスの開始時にコマンドリストへこの巨大ヒープをステージングする
		void SetGraphicsHeap(ID3D12GraphicsCommandList* cmdList);

		// Gpuハンドルを取得する
		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(uint32_t index) const;

		// インデックスからGpuハンドルを逆算する
		uint32_t GetIndex(D3D12_GPU_DESCRIPTOR_HANDLE handle) const;
	};
}