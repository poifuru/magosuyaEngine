#pragma once

namespace MyEngine::LowLevel {
	class SrvDescriptorHeap {
	public:
		static constexpr uint32_t kInvalidDescriptorIndex = UINT32_MAX;

		SrvDescriptorHeap();
		~SrvDescriptorHeap() = default;

		/// <summary>
		/// 初期化時に1つの巨大なディスクリプターヒープを作る
		/// </summary>
		/// <param name="device">デバイス</param>
		/// <param name="maxDescriptors">ディスクリプターの最大数</param>
		void Initialize(ID3D12Device* device, uint32_t maxDescriptors = 4096);

		// ディスクリプタのインデックス管理
		uint32_t AllocateIndex();
		void FreeIndex(uint32_t index);

		// 定数バッファ作成関数
		void CreateCBV(uint32_t index, const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc);

		// ビュー(SRV)生成の委譲窓口
		void CreateSRVforTexture2D(uint32_t index, ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);

		// UAV作成関数
		void CreateUAVforTexture2D(uint32_t index, ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc);

		// 描画パスの開始時にコマンドリストへこの巨大ヒープをステージングする
		void SetGraphicsHeap(ID3D12GraphicsCommandList* cmdList);

		// --- アクセッサ (エンジン層のレンダラーが使用する) --- //
		ID3D12DescriptorHeap* GetHeap() const { return heap_.Get(); }
		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32_t index) const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(uint32_t index) const;
		uint32_t GetIndex(D3D12_CPU_DESCRIPTOR_HANDLE handle) const;
		uint32_t GetIndex(D3D12_GPU_DESCRIPTOR_HANDLE handle) const;

	public:
		// コピー・移動の禁止
		SrvDescriptorHeap(const SrvDescriptorHeap&) = delete;
		SrvDescriptorHeap& operator=(const SrvDescriptorHeap&) = delete;
		SrvDescriptorHeap(SrvDescriptorHeap&&) = delete;
		SrvDescriptorHeap& operator=(SrvDescriptorHeap&&) = delete;

	private:
		uint32_t descriptorIndex_ = kInvalidDescriptorIndex;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
		uint32_t descriptorSize_ = 0;

		// インデックスのアロケーション管理用
		uint32_t maxDescriptors_ = 0;
		uint32_t nextIndex_ = 1;
		std::queue<uint32_t> freeIndices_;
	};
}