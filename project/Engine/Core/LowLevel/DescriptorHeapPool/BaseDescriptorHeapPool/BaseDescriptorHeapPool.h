#pragma once

namespace MyEngine::LowLevel {
	class BaseDescriptorHeapPool {
	public:
		// コンストラクタ
		BaseDescriptorHeapPool(
			ID3D12Device* device,
			D3D12_DESCRIPTOR_HEAP_TYPE heapType,
			uint32_t maxDescriptors,
			D3D12_DESCRIPTOR_HEAP_FLAGS isShaderVisible
		);

		// デストラクタ
		virtual ~BaseDescriptorHeapPool() = default;

		// インデックスの割り当て
		uint32_t AllocateIndex();

		// 使い終わったインデックスの解放
		void FreeIndex(uint32_t index);

		// Cpuハンドルの取得
		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32_t index) const;

		// インデックスからCpuハンドルを逆算する
		uint32_t GetIndex(D3D12_CPU_DESCRIPTOR_HANDLE handle) const;

		// ヒープの取得
		ID3D12DescriptorHeap* GetHeap() const { return heap_.Get(); }

		// ディスクリプタサイズの取得
		uint32_t GetDescriptorSize() const { return descriptorSize_; }

		// ディスクリプタ最大数の取得
		uint32_t GetMaxDescriptors() const { return maxDescriptors_; }

	protected:
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;	// ヒープそのもの
		uint32_t descriptorSize_ = 0;	// 型ごとのサイズ(バイト)
		uint32_t maxDescriptors_ = 0;	// ディスクリプタの最大数
		uint32_t nextIndex_ = 0;		// 次に割り当てられるインデックス
		std::queue<uint32_t> freeIndices_;	// アロケーション用
	};
}