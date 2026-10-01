#include "PCH.h"
#include "RenderTargetFactory.h"
#include "RenderTexture.h"
#include "DepthTexture.h"
#include "DescriptorHeapPoolContext.h"

std::unique_ptr<MyEngine::Rendering::RenderTexture> MyEngine::LowLevel::RenderTargetFactory::CreateRenderTexture(
	ID3D12Device* device,
	MyEngine::LowLevel::DescriptorHeapPoolContext* heapContext,
	uint32_t width,
	uint32_t height,
	DXGI_FORMAT format,
	const Vector4& clearColor
) {
	// nullチェック
	assert(device != nullptr);
	assert(heapContext != nullptr);

	auto* rtvPool = heapContext->GetRtvPool();
	auto* srvPool = heapContext->GetSrvPool();
	assert(rtvPool != nullptr && srvPool != nullptr);

	// テクスチャリソースの設定
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

	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	// クリア値の設定
	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = format;
	clearValue.Color[0] = clearColor.x;
	clearValue.Color[1] = clearColor.y;
	clearValue.Color[2] = clearColor.z;
	clearValue.Color[3] = clearColor.w;

	// 実際にリソースを生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&clearValue,
		IID_PPV_ARGS(&resource)
	);
	assert(SUCCEEDED(hr));

	// RTVの作成
	uint32_t rtvIndex = rtvPool->AllocateIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvPool->GetCpuHandle(rtvIndex);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = format;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	device->CreateRenderTargetView(resource.Get(), &rtvDesc, rtvHandle);

	// SRVの作成
	uint32_t srvIndex = srvPool->AllocateIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = srvPool->GetCpuHandle(srvIndex);
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = srvPool->GetGpuHandle(srvIndex);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	device->CreateShaderResourceView(resource.Get(), &srvDesc, srvCpuHandle);

	// 実際にRenderTextureを作成して返す
	return std::make_unique<MyEngine::Rendering::RenderTexture>(
		resource,
		rtvIndex,
		rtvHandle,
		srvIndex,
		srvGpuHandle
	);
}

std::unique_ptr<MyEngine::Rendering::DepthTexture> MyEngine::LowLevel::RenderTargetFactory::CreateDepthTexture(
	ID3D12Device* device,
	MyEngine::LowLevel::DescriptorHeapPoolContext* heapContext,
	uint32_t width,
	uint32_t height
) {
	// nullチェック
	assert(device != nullptr);
	assert(heapContext != nullptr);

	auto* dsvPool = heapContext->GetDsvPool();
	auto* srvPool = heapContext->GetSrvPool();
	assert(dsvPool != nullptr && srvPool != nullptr);

	// 深度テクスチャリソースの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;	// DSV,SRVの両方で使うため、リソース自体はTYPELESSに
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	// クリア値の設定
	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	// 実際にリソースを生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&resource)
	);
	assert(SUCCEEDED(hr));

	// DSVの作成
	uint32_t dsvIndex = dsvPool->AllocateIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvPool->GetCpuHandle(dsvIndex);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	device->CreateDepthStencilView(resource.Get(), &dsvDesc, dsvHandle);

	// SRVの作成
	uint32_t srvIndex = srvPool->AllocateIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = srvPool->GetCpuHandle(srvIndex);
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = srvPool->GetGpuHandle(srvIndex);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(resource.Get(), &srvDesc, srvCpuHandle);

	// 実際にDepthTexture を生成して返す
	return std::make_unique<MyEngine::Rendering::DepthTexture>(
		resource,
		dsvIndex,
		dsvHandle,
		srvIndex,
		srvGpuHandle
	);
}
