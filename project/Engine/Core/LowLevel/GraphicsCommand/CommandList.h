#pragma once

// ＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝ //
// コマンドリストの作成、記録を行う //
// ＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝ //

namespace MyEngine::LowLevel {
	class CommandList {
	public:
		CommandList() = default;
		~CommandList() = default;

		/// <summary>
		/// 初期化処理
		/// </summary>
		/// <param name="device"></param>
		void Initialize(Microsoft::WRL::ComPtr<ID3D12Device> device);

		/// <summary>
		/// アロケータとリストのリセット
		/// </summary>
		void Reset();

		/// <summary>
		/// レンダーターゲットをクリア
		/// </summary>
		/// <param name="rtvHandle"></param>
		/// <param name="color"></param>
		void ClearRenderTarget(
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
			const float color[4]
		);

		/// <summary>
		/// レンダーターゲットをセット
		/// </summary>
		/// <param name="rtvHandle"></param>
		/// <param name="dsvHandle"></param>
		void SetRenderTargets(
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
			const D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandle = nullptr
		);

		/// <summary>
		/// 深度バッファをクリア
		/// </summary>
		/// <param name="dsvHandle"></param>
		/// <param name="depth"></param>
		void ClearDepthBuffer(
			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
			float depth = 1.0f
		);

		/// <summary>
		/// コマンドアロケータを取得
		/// </summary>
		/// <returns></returns>
		ID3D12CommandAllocator* GetCommandAllocator() const { return commandAllocator_.Get(); }

		/// <summary>
		/// コマンドリストを取得
		/// </summary>
		/// <returns></returns>
		ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); }

	private:
		// コマンドアロケータ
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
		// コマンドリスト
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
	};
}