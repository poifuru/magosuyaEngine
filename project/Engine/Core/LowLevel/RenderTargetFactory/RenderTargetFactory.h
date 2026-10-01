#pragma once

namespace MyEngine::Rendering {
	class RenderTexture;
	class DepthTexture;
}

namespace MyEngine::LowLevel {
	// 前方宣言
	class DescriptorHeapPoolContext;

	class RenderTargetFactory {
	public:
		// RenderTexture生成
		static std::unique_ptr<MyEngine::Rendering::RenderTexture> CreateRenderTexture(
			ID3D12Device* device,
			MyEngine::LowLevel::DescriptorHeapPoolContext* heapContext,
			uint32_t width,
			uint32_t height,
			DXGI_FORMAT format = DXGI_FORMAT_R16G16B16A16_FLOAT,
			const Vector4& clearColor = { 0.14f, 0.14f, 0.14f, 1.0f }
		);

		// DepthTexture生成
		static std::unique_ptr<MyEngine::Rendering::DepthTexture > CreateDepthTexture(
			ID3D12Device* device,
			MyEngine::LowLevel::DescriptorHeapPoolContext* heapContext,
			uint32_t width,
			uint32_t height
		);
	};
}