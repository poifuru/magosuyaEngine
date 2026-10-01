#pragma once
#include "DsvDescriptorHeapPool.h"
#include "RtvDescriptorHeapPool.h"
#include "SrvDescriptorHeapPool.h"

namespace MyEngine::LowLevel {
	class DescriptorHeapPoolContext {
	public:
		// 3つのプールを一括初期化
		DescriptorHeapPoolContext(ID3D12Device* device);

		// デストラクタ
		~DescriptorHeapPoolContext() = default;

		// 各プールへのアクセッサ
		DsvDescriptorHeapPool* GetDsvPool() const { return dsvPool_.get(); }
		RtvDescriptorHeapPool* GetRtvPool() const { return rtvPool_.get(); }
		SrvDescriptorHeapPool* GetSrvPool() const { return srvPool_.get(); }

	private:
		std::unique_ptr<DsvDescriptorHeapPool> dsvPool_;
		std::unique_ptr<RtvDescriptorHeapPool> rtvPool_;
		std::unique_ptr<SrvDescriptorHeapPool> srvPool_;
	};
}