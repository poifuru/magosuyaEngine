#pragma once
#include "BaseDescriptorHeap.h"

namespace MyEngine::LowLevel {
	class DsvDescriptorHeap : public BaseDescriptorHeap {
		// コンストラクタ
		DsvDescriptorHeap(ID3D12Device* device, uint32_t maxDescriptors = 32);

		// デストラクタ
		~DsvDescriptorHeap() override = default;
	};
}