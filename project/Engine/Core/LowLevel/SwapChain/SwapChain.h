// =================================== //
// 画面をフリップさせるためのスワップチェイン //
// =================================== //

#pragma once

namespace MyEngine::LowLevel {
	// 前方宣言
	class CommandList;
	class RtvDescriptorHeapPool;

	class SwapChain {
	public:
		// ダブルバッファリングのためのバッファ数定数(未実装)
		static constexpr uint32_t kBufferCount = 2;

		// コンストラクタ
		SwapChain(
			ID3D12Device* device,
			IDXGIFactory7* dxgiFactory,
			ID3D12CommandQueue* cmdQueue,
			HWND hwnd,
			int32_t width,
			int32_t height,
			MyEngine::LowLevel::RtvDescriptorHeapPool* rtvPool
		);

		~SwapChain() = default;

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

		// --- アクセッサ --- //
		uint32_t GetCurrentBackBufferIndex() const { return swapChain_->GetCurrentBackBufferIndex(); }
		ID3D12Resource* GetBackBufferResource(uint32_t index) const { return swapChainResources_[index].Get(); }
		D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle(uint32_t index) const { return rtvHandles_[index]; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRtvHandle() const { return GetRtvHandle(GetCurrentBackBufferIndex()); }
		
	private:
		Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;

		std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kBufferCount> swapChainResources_;
		std::array<D3D12_CPU_DESCRIPTOR_HANDLE, kBufferCount> rtvHandles_;
		std::array<uint32_t, kBufferCount> rtvIndices_{}; // 借りたスロット番号
	};
}