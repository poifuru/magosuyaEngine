#pragma once
#include "BaseDescriptorHeapPool.h"

namespace MyEngine::LowLevel {
	class DsvDescriptorHeapPool : public BaseDescriptorHeapPool {
		// コンストラクタ
		DsvDescriptorHeapPool(ID3D12Device* device, uint32_t maxDescriptors = 32);

		// デストラクタ
		~DsvDescriptorHeapPool() override = default;
	};
}