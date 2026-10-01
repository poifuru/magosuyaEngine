#include "PCH.h"
#include "SwapChain.h"
#include "CommandList.h"
#include "Function.h"
#include "RtvDescriptorHeapPool.h"

MyEngine::LowLevel::SwapChain::SwapChain(
	ID3D12Device* device,
	IDXGIFactory7* dxgiFactory,
	ID3D12CommandQueue* cmdQueue,
	HWND hwnd,
	int32_t width,
	int32_t height,
	MyEngine::LowLevel::RtvDescriptorHeapPool* rtvPool
) {
	// もらった引数が有効かチェック
	assert(device != nullptr && dxgiFactory != nullptr && cmdQueue != nullptr && hwnd != nullptr && rtvPool != nullptr);

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
	
	// 実際に作成する
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

	// RTVの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	// 実際に作成
	for (uint32_t i = 0; i < kBufferCount; ++i) {
		hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainResources_[i]));
		assert(SUCCEEDED(hr));
		// プールからスロットとハンドルをもらう
		rtvIndices_[i] = rtvPool->AllocateIndex();
		rtvHandles_[i] = rtvPool->GetCpuHandle(rtvIndices_[i]);
		device->CreateRenderTargetView(swapChainResources_[i].Get(), &rtvDesc, rtvHandles_[i]);
	}
}

void MyEngine::LowLevel::SwapChain::Present() {
	// 垂直同期（V-Sync）を有効にするなら第1引数を 1 に、無制限にするなら 0 にする
	HRESULT hr = swapChain_->Present(1, 0);
	assert(SUCCEEDED(hr));
}

void MyEngine::LowLevel::SwapChain::BeginRender(
	MyEngine::LowLevel::CommandList* cmdList,
	const float clearColor[4]
) {
	uint32_t bbIndex = GetCurrentBackBufferIndex();
	ID3D12Resource* backBuffer = GetBackBufferResource(bbIndex);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRtvHandle(bbIndex);

	MyEngine::Utility::TransitionBarrier(
		cmdList->GetCommandList(),
		backBuffer, 
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	
	cmdList->SetRenderTargets(rtvHandle, nullptr);
	cmdList->ClearRenderTarget(rtvHandle, clearColor);
}

void MyEngine::LowLevel::SwapChain::EndRender(MyEngine::LowLevel::CommandList* cmdList) {
	uint32_t bbIndex = GetCurrentBackBufferIndex();
	ID3D12Resource* backBuffer = GetBackBufferResource(bbIndex);

	MyEngine::Utility::TransitionBarrier(cmdList->GetCommandList(), backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}

void MyEngine::LowLevel::SwapChain::Resize(uint32_t width, uint32_t height) {
	if (width <= 0 || height <= 0) return;	// 最小化されたときのガード

	// デバイスを取得
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	HRESULT hr = swapChain_->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));
	assert(SUCCEEDED(hr));

	// 既存のバックバッファリソースをすべて解放する
	for (uint32_t i = 0; i < kBufferCount; ++i) {
		swapChainResources_[i].Reset();
	}
	
	// スワップチェーンのバッファサイズを変更
	hr = swapChain_->ResizeBuffers(
		kBufferCount,
		width,
		height,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		0
	);
	assert(SUCCEEDED(hr));

	// RTV（レンダーターゲットビュー）の再設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	// 実際に再生成
	for (uint32_t i = 0; i < kBufferCount; ++i) {
		hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainResources_[i]));
		assert(SUCCEEDED(hr));
		device->CreateRenderTargetView(swapChainResources_[i].Get(), &rtvDesc, rtvHandles_[i]);
	}
}