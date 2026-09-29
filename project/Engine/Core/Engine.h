#pragma once
#include "LeakChecker.h"

// 前方宣言
namespace MyEngine::LowLevel {
	class FrameRateController;
	class GraphicsDevice;
	class DxcCompiler;
	class CommandQueue;
	class CommandList;
	class SwapChain;
	class SrvDescriptorHeapPool;
}

namespace MyEngine::Rendering {
	class RenderTexture;
}

namespace MyEngine::LowLevel {
	class Engine {
	public:
		Engine();
		~Engine();

		/// <summary>
		/// 初期化
		/// </summary>
		void Initialize();

		/// <summary>
		/// ウィンドウからのメッセージを処理する
		/// </summary>
		/// <returns></returns>
		bool ProcessMessage();

		/// <summary>
		/// 描画前処理
		/// </summary>
		/// <param name="renderTexture"></param>
		/// <param name="renderTexDescriptorHandle"></param>
		void BeginFrame(D3D12_CPU_DESCRIPTOR_HANDLE renderTexDescriptorHandle);

		/// <summary>
		/// 描画後処理
		/// </summary>
		void EndFrame();

		/// <summary>
		/// ImGui描画前処理
		/// </summary>
		void BeginSwapChainRender();

		// コマンド制御
		void ResetCommandList();
		void ExecuteCommandList();

		/// <summary>
		/// ウィンドウサイズの変更を通知する
		/// </summary>
		/// <param name="width"></param>
		/// <param name="height"></param>
		void OnResize(uint32_t width, uint32_t height);

		// プロファイル時間取得・設定用
		static float GetUpdateTime() { return sUpdateTime_; }
		static float GetRenderTime() { return sRenderTime_; }
		static float GetGpuWaitTime() { return sGpuWaitTime_; }

		static void SetUpdateTime(float time) { sUpdateTime_ = time; }
		static void SetRenderTime(float time) { sRenderTime_ = time; }
		static void SetGpuWaitTime(float time) { sGpuWaitTime_ = time; }

		// 低レイヤーアクセッサ
		ID3D12Device* GetDevice();
		MyEngine::LowLevel::GraphicsDevice* GetGraphicsDevice();
		ID3D12GraphicsCommandList* GetCommandList();
		ID3D12CommandQueue* GetCommandQueue();
		IDxcUtils* GetDxcUtils();
		IDxcCompiler3* GetDxcCompiler();
		IDxcIncludeHandler* GetIncludeHandler();

		MyEngine::LowLevel::SrvDescriptorHeapPool* GetSrvDescriptorHeapPool() { return heapManager_.get(); }
		MyEngine::LowLevel::SwapChain* GetSwapChain() { return swapChain_.get(); }

	private:
		LeakChecker leakCheck_{};

		// ---上から順に初期化、下から順に破棄--- //
		MyEngine::LowLevel::FrameRateController* frameRateController_ = nullptr;
		std::unique_ptr<MyEngine::LowLevel::GraphicsDevice> device_;
		std::unique_ptr<MyEngine::LowLevel::DxcCompiler> dxcCompiler_;
		std::unique_ptr<MyEngine::LowLevel::CommandQueue> cmdQueue_;
		std::unique_ptr<MyEngine::LowLevel::CommandList> cmdList_;
		std::unique_ptr<MyEngine::LowLevel::SwapChain> swapChain_;
		std::unique_ptr<MyEngine::LowLevel::SrvDescriptorHeapPool> heapManager_;

		// 静的プロファイル用変数
		static float sUpdateTime_;
		static float sRenderTime_;
		static float sGpuWaitTime_;
 
 		// 遅延リサイズ用の予約フラグとサイズ
 		bool resizeRequested_ = false;
 		uint32_t newWidth_ = 0;
 		uint32_t newHeight_ = 0;
 	};
}