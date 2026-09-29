#pragma once
#include "struct.h"
#include "ConstantBuffer.h"
#include "RenderCommand.h"

// 前方宣言
namespace MyEngine::LowLevel{
	class SrvDescriptorHeap;
}
namespace MyEngine::Rendering {
	class PSOManager;
	class ShaderManager;
	class InputLayoutManager;
	class BlendModeManager;
}

class LightManager;
struct CameraForGPU {
	Vector3 worldPosition;
	float padding; // 16バイトアライメント用のパディング
};

namespace MyEngine::Rendering {
	class RenderSystem {
	public:
		RenderSystem() = default;
		~RenderSystem() = default;

		// リソース初期化用
		void Initialize(
			ID3D12Device* device,
			MyEngine::LowLevel::SrvDescriptorHeap* heapManager,
			PSOManager* psoManager,
			const ShaderManager* shaderManager,
			const InputLayoutManager* inputLayoutManager,
			const BlendModeManager* blendModeManager,
			ID3D12RootSignature* commonRootSignature
		);

		// コマンドの積み込み
		void PushCommand(const RenderCommand& command);

		// カメラ位置の設定・更新用
		void SetCameraPosition(const Vector3& cameraPos);

		// アクティブなライトマネージャーの登録用
		void SetLightManager(LightManager* lightManager);

		void WriteCommandList(ID3D12GraphicsCommandList* cmdList);

		void ClearCommands();

		// 一時的なアクセッサ
		ID3D12Device* GetDevice() { return device_; }

		// ドローコール数取得用
		static uint32_t GetDrawCallCount() { return sDrawCallCount_; }

	public:
		// コピー・移動禁止
		RenderSystem(const RenderSystem&) = delete;
		RenderSystem& operator=(const RenderSystem&) = delete;
		RenderSystem(RenderSystem&&) = delete;
		RenderSystem& operator=(RenderSystem&&) = delete;

	private:
		ID3D12Device* device_ = nullptr;
		MyEngine::LowLevel::SrvDescriptorHeap* heapManager_ = nullptr;
		PSOManager* psoManager_ = nullptr;
		const ShaderManager* shaderManager_ = nullptr;
		const InputLayoutManager* inputLayoutManager_ = nullptr;
		const BlendModeManager* blendModeManager_ = nullptr;
		ID3D12RootSignature* commonRootSignature_ = nullptr;

		std::vector<RenderCommand> commandQueue_;

		// カメラ用の定数バッファ
		ConstantBuffer<CameraForGPU> cameraBuffer_;

		// アクティブなライトマネージャーへのポインタ
		LightManager* activeLightManager_ = nullptr;

		// ドローコール数カウンタ
		static uint32_t sDrawCallCount_;
	};
}