#pragma once

// ＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝ //
// コマンドキューの保持、同期を行う //
// ＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝ //

namespace MyEngine::LowLevel {
	class CommandQueue {
	public:
		CommandQueue() = default;
		~CommandQueue();

		/// <summary>
		/// 初期化処理
		/// </summary>
		/// <param name="device"></param>
		void Initialize(ID3D12Device* device);

		/// <summary>
		/// コマンドを実行する
		/// </summary>
		/// <param name="cmdList"></param>
		void ExecuteCommandList(ID3D12GraphicsCommandList* cmdList);

		/// <summary>
		/// コマンドキューとGPUの同期
		/// </summary>
		void SignalAndWait();

		/// <summary>
		/// コマンドキューを取得
		/// </summary>
		/// <returns></returns>
		ID3D12CommandQueue* GetCommandQueue() const { return commandQueue_.Get(); }

	private:
		// コマンドキュー
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;

		// 同期（フェンス）用
		Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
		uint64_t fenceValue_ = 0;
		HANDLE fenceEvent_ = nullptr;
	};
}