#pragma once

namespace MyEngine::Rendering {
	class DepthTexture {
	public:
		// コンストラクタ
		DepthTexture(
			Microsoft::WRL::ComPtr<ID3D12Resource> resource,
			uint32_t dsvIndex,
			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
			uint32_t srvIndex,
			D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle
		);

		// デストラクタ
		~DepthTexture() = default;

		// バリア状態を変更する
		void ChangeState(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState);

		// アクセッサ
		ID3D12Resource* GetResource() const { return resource_.Get(); }	// リソース
		D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle() const { return dsvHandle_; } // DSV用
		uint32_t GetDsvIndex() const { return dsvIndex_; }
		D3D12_GPU_DESCRIPTOR_HANDLE GetSrvGpuHandle() const { return srvGpuHandle_; } // SRV用
		uint32_t GetSrvIndex() const { return srvIndex_; }

	private:
		// リソース
		Microsoft::WRL::ComPtr<ID3D12Resource> resource_ = nullptr;

		// DSV情報
		uint32_t dsvIndex_ = UINT32_MAX;
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle_{};

		// SRV情報
		uint32_t srvIndex_ = UINT32_MAX;
		D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle_{};

		// 初期状態は深度書き込み（DEPTH_WRITE）
		D3D12_RESOURCE_STATES currentState_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	};
}
