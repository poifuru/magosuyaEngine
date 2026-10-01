#pragma once

namespace MyEngine::LowLevel {
	class RtvDescriptorHeapPool;
	class SrvDescriptorHeapPool;
}

namespace MyEngine::Rendering {
	class RenderTexture {
	public:
		// コンストラクタ
		RenderTexture(
			Microsoft::WRL::ComPtr<ID3D12Resource> resource,
			uint32_t rtvIndex,
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
			uint32_t srvIndex,
			D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle
		);

		// デストラクタ
		~RenderTexture() = default;

		/// <summary>
		/// RenderTexture生成関数
		/// </summary>
		Microsoft::WRL::ComPtr<ID3D12Resource> CreateRenderTextureResource(
			ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format, const Vector4& clearColor
		);

		// バリア状態を変更する
		void ChangeState(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState);

		// アクセッサ
		ID3D12Resource* GetResource() const { return resource_.Get(); }	// リソース
		uint32_t GetRtvIndex() const { return rtvIndex_; }	// RTV用
		D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle() const { return rtvHandle_; }
		uint32_t GetSrvIndex() const { return srvIndex_; }	// SRV用
		D3D12_GPU_DESCRIPTOR_HANDLE GetSrvGpuHandle() const { return srvGpuHandle_; }

	private:
		// RenderTextureのリソース
		Microsoft::WRL::ComPtr<ID3D12Resource> resource_ = nullptr;

		// Rtv情報
		uint32_t rtvIndex_ = UINT32_MAX;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_{};

		// Srv情報
		uint32_t srvIndex_ = UINT32_MAX;
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle_{};

		// バリア状態(初期状態は PIXEL_SHADER_RESOURCE ）
		D3D12_RESOURCE_STATES currentState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
	};
}