#pragma once

// 前方宣言
namespace MyEngine::LowLevel {
	class SrvDescriptorHeap;
}

class RWTexture2D {
public:
	RWTexture2D() = default;
	~RWTexture2D() { Release(); }

	// 2Dテクスチャの解像度とフォーマットを指定して初期化
	void Initialize(
		ID3D12Device* device,
		MyEngine::LowLevel::SrvDescriptorHeap& heapManager,
		uint32_t width,
		uint32_t height,
		DXGI_FORMAT format = DXGI_FORMAT_R16G16B16A16_FLOAT
	);

	void Release();

	void Transition(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES stateAfter);
	
	// アクセッサ
	ID3D12Resource* GetResource() const { return resource_.Get(); }
	uint32_t GetSrvIndex() const { return srvDescriptorIndex_; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetUavGpuHandle() const { return uavGpuHandle_; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetUavCpuHandle() const { return uavCpuHandle_; }
	uint32_t GetWidth() const { return width_; }
	uint32_t GetHeight() const { return height_; }

public:
	RWTexture2D(const RWTexture2D&) = delete;
	RWTexture2D& operator=(const RWTexture2D&) = delete;

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> resource_ = nullptr;

	uint32_t width_ = 0;
	uint32_t height_ = 0;
	DXGI_FORMAT format_ = DXGI_FORMAT_R16G16B16A16_FLOAT;
	D3D12_RESOURCE_STATES currentState_ = D3D12_RESOURCE_STATE_COMMON;

	// バインドレスヒープ用のSRVインデックス
	uint32_t srvDescriptorIndex_ = 0;

	// CSで使用するUAVのデスクリプタハンドル
	D3D12_CPU_DESCRIPTOR_HANDLE uavCpuHandle_{};
	D3D12_GPU_DESCRIPTOR_HANDLE uavGpuHandle_{};
	uint32_t uavDescriptorIndex_ = 0;
	MyEngine::LowLevel::SrvDescriptorHeap* heapManager_ = nullptr;
};