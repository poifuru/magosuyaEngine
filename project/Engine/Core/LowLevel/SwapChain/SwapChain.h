// =================================== //
// 画面をフリップさせるためのスワップチェイン //
// =================================== //

#pragma once

// 前方宣言
namespace MyEngine::LowLevel {
	class CommandList;
	class SrvDescriptorHeapPool;
}

namespace MyEngine::LowLevel {
	class SwapChain {
	public:
		// トリプルバッファリングのためのバッファ数定数
		static constexpr uint32_t kBufferCount = 3;

		SwapChain();
		~SwapChain() = default;

		/// <summary>
		/// 初期化処理
		/// </summary>
		/// <param name="dxgiFactory">DXGIファクトリー</param>
		/// <param name="cmdQueue">コマンドキュー</param>
		/// <param name="hwnd">ウィンドウハンドル</param>
		/// <param name="width">ウィンドウサイズ(横)</param>
		/// <param name="height">ウィンドウサイズ(縦)</param>
		void Initialize(
			IDXGIFactory7* dxgiFactory,
			ID3D12CommandQueue* cmdQueue,
			HWND hwnd,
			int32_t width,
			int32_t height
		);

		/// <summary>
		/// 画面をフリップさせる
		/// </summary>
		void Present();

		/// <summary>
		/// 描画開始処理（バリア遷移・クリア・ターゲット設定をまとめて行う）
		/// </summary>
		void BeginRender(MyEngine::LowLevel::CommandList* cmdList, const float clearColor[4]);

		/// <summary>
		/// 描画終了処理（バリアをPRESENTに戻す）
		/// </summary>
		void EndRender(MyEngine::LowLevel::CommandList* cmdList);

		void Resize(uint32_t width, uint32_t height);

		void CreateDepthSRV(ID3D12Device* device, MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager);

		// --- アクセッサ --- //
		uint32_t GetCurrentBackBufferIndex() const { return swapChain_->GetCurrentBackBufferIndex(); }
		ID3D12Resource* GetBackBufferResource(uint32_t index) const { return swapChainResources_[index].Get(); }
		D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle(uint32_t index) const { return rtvHandles_[index]; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRtvHandle() const { return GetRtvHandle(GetCurrentBackBufferIndex()); }
		D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle() const { return dsvHandle_; }
		uint32_t GetDepthSrvIndex() const { return dsvSrvIndex_; }
		ID3D12Resource* GetDepthBufferResource() const { return depthBuffer_.Get(); }

	public:
		// コピー・移動禁止
		SwapChain(const SwapChain&) = delete;
		SwapChain& operator=(const SwapChain&) = delete;
		SwapChain(SwapChain&&) = delete;
		SwapChain& operator=(SwapChain&&) = delete;

	private:
		Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;

		std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kBufferCount> swapChainResources_;
		std::array<D3D12_CPU_DESCRIPTOR_HANDLE, kBufferCount> rtvHandles_;

		// RTV専用のディスクリプタヒープ
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;

		// デプスバッファ関連
		Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer_;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle_{};
		uint32_t dsvSrvIndex_ = 0; // 深度SRVインデックス

		// ポインタ
		MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager_;
	};
}