#pragma once
#include "BaseDescriptorHeapPool.h"

namespace MyEngine::LowLevel {
	class RtvDescriptorHeapPool : public BaseDescriptorHeapPool {
	public:
		// コンストラクタ
		RtvDescriptorHeapPool(ID3D12Device* device, uint32_t maxDescriptors = 64);

		// デストラクタ
		~RtvDescriptorHeapPool() override = default;
	};
}