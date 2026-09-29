#pragma once

namespace MyEngine::LowLevel {
	class SrvDescriptorHeapPool;
}

namespace MyEngine::Rendering {
	class RenderTexture {
	public:
		RenderTexture() = default;
		~RenderTexture();

		/// <summary>
		/// 初期化
		/// </summary>
		void Initialize(ID3D12Device* device, MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager);

		/// <summary>
		/// RenderTexture生成関数
		/// </summary>
		Microsoft::WRL::ComPtr<ID3D12Resource> CreateRenderTextureResource(
			ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format, const Vector4& clearColor
		);

		// 状態を安全に変更する関数
		void ChangeState(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState);

		// レンダーテクスチャサイズを変更する
		void Resize(ID3D12Device* device, uint32_t width, uint32_t height);

		// アクセッサ
		ID3D12Resource* GetResource() const { return resource_.Get(); }
		D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle() const { return rtvHandle_; }
		uint32_t GetSrvIndex() const { return srvIndex_; }

	private:
		void Release();

	private:
		// RenderTextureのリソース
		Microsoft::WRL::ComPtr<ID3D12Resource> resource_ = nullptr;

		// RTV専用のディスクリプタヒープ
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_ = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_{};

		// シェーダーマネージャーに渡すときに使うインデックス
		uint32_t srvIndex_ = 0;

		// 初期状態を覚えておく（作成時は PIXEL_SHADER_RESOURCE ）
		D3D12_RESOURCE_STATES currentState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;

		MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager_ = nullptr;
	};
}