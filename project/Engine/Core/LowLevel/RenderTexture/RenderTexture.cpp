#include "PCH.h"
#include "RenderTexture.h"
#include "SrvDescriptorHeapPool.h"
#include "Function.h"
#include "WindowsAPI.h"

MyEngine::Rendering::RenderTexture::~RenderTexture() {
	Release();
}

void MyEngine::Rendering::RenderTexture::Initialize(ID3D12Device* device, MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager) {
	heapManager_ = heapManager;

	// オフスクリーンレンダリング用のクリアカラー
	const Vector4 kRenderTargetClearValue{ 0.14f, 0.14f, 0.14f, 1.0f }; // SwapChainのClear色と合わせる

	// フォーマット(リニアワークフロー用)
	DXGI_FORMAT renderFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	// リソース作成
	CreateRenderTextureResource(
		device,
		WindowsAPI::GetInstance()->GetWindowWidth(),
		WindowsAPI::GetInstance()->GetWindowHeight(),
		renderFormat,
		kRenderTargetClearValue
	);

	// RTV用のヒープを1つだけ作成
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = 1;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	HRESULT hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_));
	assert(SUCCEEDED(hr));

	rtvHandle_ = rtvHeap_->GetCPUDescriptorHandleForHeapStart();

	// RTV作成
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = renderFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	device->CreateRenderTargetView(resource_.Get(), &rtvDesc, rtvHandle_);

	// SRV作成
	srvIndex_ = heapManager->AllocateIndex();
	
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = renderFormat;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	heapManager->CreateSRVforTexture2D(srvIndex_, resource_.Get(), srvDesc);
}

Microsoft::WRL::ComPtr<ID3D12Resource> MyEngine::Rendering::RenderTexture::CreateRenderTextureResource(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format, const Vector4& clearColor) {
	// Resourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	// ヒープの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	// クリアカラーを設定
	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = format;
	clearValue.Color[0] = clearColor.x;
	clearValue.Color[1] = clearColor.y;
	clearValue.Color[2] = clearColor.z;
	clearValue.Color[3] = clearColor.w;

	// Resourceの生成
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET  ,	// 最初はPIXEL_SHADER_RESOURCE として作っておく
		&clearValue,	// Clear最適値。ClearRenderTargetをこの色でClearするようにする。
		IID_PPV_ARGS(&resource_)
	);
	assert(SUCCEEDED(hr));

	// 作ったresourceを返す
	return resource_;
}

void MyEngine::Rendering::RenderTexture::ChangeState(
	ID3D12GraphicsCommandList* cmdList,
	D3D12_RESOURCE_STATES newState
) {
		if (currentState_ != newState) {
			MyEngine::Utility::TransitionBarrier(cmdList, resource_.Get(), currentState_, newState);
			currentState_ = newState; // 現在の状態を更新！
		}
}

void MyEngine::Rendering::RenderTexture::Resize(ID3D12Device* device, uint32_t width, uint32_t height) {
	if (!resource_) return;

	// 古いGPUリソースを解放
	resource_.Reset();

	// 変更後のサイズでリソースを再生成
	const Vector4 kRenderTargetClearValue{ 0.14f, 0.14f, 0.14f, 1.0f };
	DXGI_FORMAT renderFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	CreateRenderTextureResource(device, width, height, renderFormat, kRenderTargetClearValue);

	// RTVを再生成
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = renderFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	device->CreateRenderTargetView(resource_.Get(), &rtvDesc, rtvHandle_);

	// 既存の srvIndex_ の場所に新しいリソースのSRVを上書き
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = renderFormat;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	heapManager_->CreateSRVforTexture2D(srvIndex_, resource_.Get(), srvDesc);
}

void MyEngine::Rendering::RenderTexture::Release() {
	if (heapManager_ && srvIndex_ != 0) {
		heapManager_->FreeIndex(srvIndex_); // ヒープのインデックスを返却
		srvIndex_ = 0;
	}
	resource_.Reset();
	rtvHeap_.Reset();
}