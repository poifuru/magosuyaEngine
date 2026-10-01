#pragma once
#include "GameObject.h"

namespace MyEngine::LowLevel {
	class DescriptorHeapPoolContext;;
	class SwapChain;
}

class PostEffectManager;

namespace MyEngine::Rendering {
	class RenderTexture;
	class DepthTexture;
	class RenderSystem;
	class RootSignatureManager;
	class ShaderManager;
	class InputLayoutManager;
	class BlendModeManager;
	class PSOManager;

	struct Mesh;
	class Material;

	enum class ShadingModel : uint8_t;
	enum class BlendModeType : uint8_t;
	enum class InputLayoutType : uint32_t;

	struct ShaderPair {
		uint32_t vs_ID;
		uint32_t ps_ID;
	};

	class Renderer {
	public:
		// コンストラクタ
		Renderer(
			ID3D12Device* device,
			IDxcUtils* dxcUtils,
			IDxcCompiler3* dxcCompiler,
			IDxcIncludeHandler* includeHandler,
			MyEngine::LowLevel::DescriptorHeapPoolContext* heapContext,
			MyEngine::LowLevel::SwapChain* swapChain
		);

		// デストラクタ
		~Renderer();

		void RenderScene(
			ID3D12GraphicsCommandList* cmdList,
			MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager
		);

		void DispatchCS(
			MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager
		);

		void Draw(std::vector<std::unique_ptr<GameObject>>& objects);

		void ExecutePostProcess(PostEffectManager* postEffectManager);

		MyEngine::Rendering::RenderSystem* GetRenderSystem() { return renderSystem_.get(); }
		MyEngine::Rendering::RenderTexture* GetRenderTexture() { return renderTexture_.get(); }
		MyEngine::Rendering::RootSignatureManager* GetRootSigManager() { return rootSigManager_.get(); }
		MyEngine::Rendering::PSOManager* GetPSOManager() { return psoManager_.get(); }
		MyEngine::Rendering::ShaderManager* GetShaderManager() { return shaderManager_.get(); }
		MyEngine::Rendering::InputLayoutManager* GetInputLayoutManager() { return inputLayoutManager_.get(); }
		MyEngine::Rendering::BlendModeManager* GetBlendModeManager() { return blendModeManager_.get(); }

		void SetPostEffectManager(PostEffectManager* postEffectManager) { postEffectManager_ = postEffectManager; }

		// 最終的な描画結果の RenderTexture を取得するゲッター
		RenderTexture* GetFinalRenderTexture() const {
			return finalRenderTexture_ ? finalRenderTexture_ : renderTexture_.get();
		}

	private:
		uint64_t MakeShaderKey(
			MyEngine::Rendering::ShadingModel model,
			MyEngine::Rendering::InputLayoutType layout
		);

		void InitializeShaderTable();

		void Submit(
			const MyEngine::Rendering::Mesh& mesh,
			MyEngine::Rendering::Material* material,
			D3D12_GPU_VIRTUAL_ADDRESS transformAddr,
			const char* debugName,
			D3D12_GPU_VIRTUAL_ADDRESS customAddr = 0
		);

		void SubmitPostEffect(
			ID3D12GraphicsCommandList* cmdList,
			MyEngine::Rendering::ShadingModel model,
			D3D12_GPU_VIRTUAL_ADDRESS cbvAddress,
			uint32_t srcTextureSrvIndex,
			uint32_t extraSrvIndex
		);

		// ピンポンレンダリング用のヘルパー関数
		void Pingpong(PostEffectManager* postEffectManager);
		void SetBackBufferAsRenderTarget();

		// ビューポートとシザーレクトを現在の解像度に合わせる
		void SetViewportAndScissorRect();

	private:
		ID3D12Device* device_ = nullptr;
		ID3D12GraphicsCommandList* cmdList_ = nullptr;
		MyEngine::LowLevel::DescriptorHeapPoolContext* heapContext_ = nullptr;
		MyEngine::LowLevel::SwapChain* swapChain_ = nullptr;

		std::unique_ptr<MyEngine::Rendering::RootSignatureManager> rootSigManager_;
		std::unique_ptr<MyEngine::Rendering::ShaderManager> shaderManager_;
		std::unique_ptr<MyEngine::Rendering::InputLayoutManager> inputLayoutManager_;
		std::unique_ptr<MyEngine::Rendering::BlendModeManager> blendModeManager_;
		std::unique_ptr<MyEngine::Rendering::PSOManager> psoManager_;

		std::unique_ptr<MyEngine::Rendering::RenderSystem> renderSystem_;
		std::unique_ptr<MyEngine::Rendering::RenderTexture> renderTexture_;
		std::unique_ptr<MyEngine::Rendering::DepthTexture> sceneDepthTexture_;
		// ピンポン用の中間テクスチャ2枚
		std::unique_ptr<MyEngine::Rendering::RenderTexture> workTextures_[2];

		// シェーダーの組み合わせを保存しておく
		std::unordered_map<uint64_t, ShaderPair> shaderTable_;

		// objectsの参照を覚えておく
		std::vector<std::unique_ptr<GameObject>>* currentObjects_ = nullptr;

		// PostEffectManagerの参照
		PostEffectManager* postEffectManager_ = nullptr;

		// ポストエフェクト適用後の最終テクスチャを指すポインタ
		RenderTexture* finalRenderTexture_ = nullptr;
		bool lastEffect_ = false;
	};
}