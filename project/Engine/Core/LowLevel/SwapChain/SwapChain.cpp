#include "PCH.h"
#include "SwapChain.h"
#include "CommandList.h"
#include "Function.h"
#include "SrvDescriptorHeap.h"

MyEngine::LowLevel::SwapChain::SwapChain() = default;

void MyEngine::LowLevel::SwapChain::Initialize(
	IDXGIFactory7* dxgiFactory,
	ID3D12CommandQueue* cmdQueue,
	HWND hwnd,
	int32_t width,
	int32_t height) {
	// もらった引数が有効かチェック
	assert(dxgiFactory != nullptr && cmdQueue != nullptr && hwnd != nullptr);

	HRESULT hr = S_OK;

	// スワップチェーンの設定
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; // HDR対応させる
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = kBufferCount;	// 3つ分
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Flags = 0;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
	hr = dxgiFactory->CreateSwapChainForHwnd(
		cmdQueue,
		hwnd,
		&swapChainDesc,
		nullptr, nullptr,
		swapChain1.GetAddressOf()
	);
	assert(SUCCEEDED(hr));

	// 4にキャストして保持
	hr = swapChain1.As(&swapChain_);
	assert(SUCCEEDED(hr));

	// IDXGISwapChain3へインターフェースを取得
	Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain3;
	if(SUCCEEDED(swapChain_->QueryInterface(IID_PPV_ARGS(&swapChain3)))) {
		// sRGB(Linear FP16)用のColorSpace
		DXGI_COLOR_SPACE_TYPE colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;

		UINT colorSpaceSupport = 0;
		if(SUCCEEDED(swapChain3->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport)) &&
		   (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)) {

			// WindowsにこのスワップチェーンはscRGB HDRであることを伝える
			swapChain3->SetColorSpace1(colorSpace);
		}
	}

	// RTV用のディスクリプタヒープを作成（数は3つ分）
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NumDescriptors = kBufferCount; // 3つ分
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // シェーダーからは見せない

	// 瞬間的に生デバイスを取得するために、一時的にcommandQueueからDeviceを引っ張り出す（裏技的だけど安全）
	Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice;
	hr = cmdQueue->GetDevice(IID_PPV_ARGS(d3dDevice.GetAddressOf()));
	assert(SUCCEEDED(hr));

	hr = d3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(rtvHeap_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	uint32_t rtvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// バックバッファリソースの取得とRTVの生成
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; // ガンマ補正を自動でかけるためSRGB
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandleStart = rtvHeap_->GetCPUDescriptorHandleForHeapStart();

	for (uint32_t i = 0; i < kBufferCount; ++i) {
		// スワップチェーンからバッファ（ID3D12Resource）を引き出す
		hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainResources_[i]));
		assert(SUCCEEDED(hr));

		// ハンドル位置を計算
		rtvHandles_[i] = rtvHandleStart;
		rtvHandles_[i].ptr += static_cast<SIZE_T>(i) * rtvDescriptorSize;

		// レンダーターゲットビューの生成
		d3dDevice->CreateRenderTargetView(swapChainResources_[i].Get(), &rtvDesc, rtvHandles_[i]);
	}

	// 深度バッファの作成
	depthBuffer_ = MyEngine::Utility::CreateDepthStencilTextureResource(d3dDevice.Get(), width, height);

	// DSV用ディスクリプタヒープの作成
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = d3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(dsvHeap_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	// DSVの作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvHandle_ = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
	d3dDevice->CreateDepthStencilView(depthBuffer_.Get(), &dsvDesc, dsvHandle_);
}

void MyEngine::LowLevel::SwapChain::Present() {
	// 垂直同期（V-Sync）を有効にするなら第1引数を 1 に、無制限にするなら 0 にする
	HRESULT hr = swapChain_->Present(1, 0);
	assert(SUCCEEDED(hr));
}

void MyEngine::LowLevel::SwapChain::BeginRender(MyEngine::LowLevel::CommandList* cmdList, const float clearColor[4]) {
	uint32_t bbIndex = GetCurrentBackBufferIndex();
	ID3D12Resource* backBuffer = GetBackBufferResource(bbIndex);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRtvHandle(bbIndex);

	MyEngine::Utility::TransitionBarrier(
		cmdList->GetCommandList(),
		backBuffer, 
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	
	cmdList->SetRenderTargets(rtvHandle, &dsvHandle_);
	cmdList->ClearRenderTarget(rtvHandle, clearColor);
	cmdList->ClearDepthBuffer(dsvHandle_, 1.0f);
}

void MyEngine::LowLevel::SwapChain::EndRender(MyEngine::LowLevel::CommandList* cmdList) {
	uint32_t bbIndex = GetCurrentBackBufferIndex();
	ID3D12Resource* backBuffer = GetBackBufferResource(bbIndex);

	MyEngine::Utility::TransitionBarrier(cmdList->GetCommandList(), backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}

void MyEngine::LowLevel::SwapChain::Resize(uint32_t width, uint32_t height) {
	if (width <= 0 || height <= 0) return;	// 最小化されたときのガード

	// デバイスを取得
	Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice;
	HRESULT hr = swapChain_->GetDevice(IID_PPV_ARGS(d3dDevice.GetAddressOf()));
	assert(SUCCEEDED(hr));

	// 既存のバックバッファと深度バッファのリソースをすべて解放する
	for (uint32_t i = 0; i < kBufferCount; ++i) {
		swapChainResources_[i].Reset();
	}
	depthBuffer_.Reset();

	// スワップチェーンのバッファサイズを変更
	hr = swapChain_->ResizeBuffers(
		kBufferCount,
		width,
		height,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		0
	);
	assert(SUCCEEDED(hr));

	// RTV（レンダーターゲットビュー）の再生成
	uint32_t rtvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandleStart = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	for (uint32_t i = 0; i < kBufferCount; ++i) {
		hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainResources_[i]));
		assert(SUCCEEDED(hr));
		rtvHandles_[i] = rtvHandleStart;
		rtvHandles_[i].ptr += static_cast<SIZE_T>(i) * rtvDescriptorSize;
		d3dDevice->CreateRenderTargetView(swapChainResources_[i].Get(), &rtvDesc, rtvHandles_[i]);
	}

	// 新しいサイズで深度バッファを再生成
	depthBuffer_ = MyEngine::Utility::CreateDepthStencilTextureResource(d3dDevice.Get(), width, height);
	// DSV（デプスステンシルビュー）の再生成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	d3dDevice->CreateDepthStencilView(depthBuffer_.Get(), &dsvDesc, dsvHandle_);

	if (dsvSrvIndex_ != 0) {
		// 再生成された depthBuffer_ で SRV を更新
		// （heapManager の参照が必要なため、呼び出し元で更新するか、heapManager を保持させて呼ぶ）
	}
}

void MyEngine::LowLevel::SwapChain::CreateDepthSRV(
	ID3D12Device* device, 
	MyEngine::LowLevel::SrvDescriptorHeap* heapManager
){
	heapManager_ = heapManager;

	if (dsvSrvIndex_ == 0) {
		dsvSrvIndex_ = heapManager->AllocateIndex();
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	heapManager->CreateSRVforTexture2D(dsvSrvIndex_, depthBuffer_.Get(), srvDesc);
}