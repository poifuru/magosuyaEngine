#pragma once
#include "BaseDescriptorHeap.h"

namespace MyEngine::LowLevel {
	class RtvDescriptorHeap : public BaseDescriptorHeap {
	public:
		// コンストラクタ
		RtvDescriptorHeap(ID3D12Device* device, uint32_t maxDescriptors = 64);

		// デストラクタ
		~RtvDescriptorHeap() override = default;
	};
}