#include "PCH.h"
#include "BaseDescriptorHeapPool.h"

MyEngine::LowLevel::BaseDescriptorHeapPool::BaseDescriptorHeapPool(
	ID3D12Device* device,
	D3D12_DESCRIPTOR_HEAP_TYPE heapType,
	uint32_t maxDescriptors,
	D3D12_DESCRIPTOR_HEAP_FLAGS isShaderVisible
) {
	assert(device != nullptr);

	maxDescriptors_ = maxDescriptors;

	// ディスクリプタサイズ(一個当たり何バイト進めばいいのか)を取得
	descriptorSize_ = device->GetDescriptorHandleIncrementSize(heapType);

	Logger::GetInstance()->Log(
		std::format(
			"-----ディスクリプタヒープ作成開始-----\n Type: {}\n, MaxDescriptors: {}\n, Size: {} bytes\n",
			(int)heapType, maxDescriptors_, descriptorSize_
		)
	);

	// 巨大なディスクリプタヒープの作成設定
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = heapType;
	heapDesc.NumDescriptors = maxDescriptors_;
	heapDesc.Flags = isShaderVisible;
	heapDesc.NodeMask = 0;

	HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(heap_.GetAddressOf()));

	if(SUCCEEDED(hr)) {
		Logger::GetInstance()->Log("作成に成功しました。");
	}
	else {
		Logger::GetInstance()->Log("作成に失敗しました。");
		assert(false && "ディスクリプタヒープの作成に失敗しました。");
	}
}

uint32_t MyEngine::LowLevel::BaseDescriptorHeapPool::AllocateIndex() {
	// 返却された空き枠(キュー)があれば、優先的にそこを再利用する
	if(!freeIndices_.empty()) {
		uint32_t index = freeIndices_.front();
		freeIndices_.pop();
		return index;
	}

	// 新しいインデックスを切り出す(上限チェック)
	if (nextIndex_ >= maxDescriptors_) {
		Logger::GetInstance()->Log("BaseDescriptorHeap ERROR: ヒープの上限を超えました\n");
		assert(false && "ディスクリプタヒープの最大確保数を超えました。");
	}

	uint32_t index = nextIndex_;
	nextIndex_++;

	return index;
}

void MyEngine::LowLevel::BaseDescriptorHeapPool::FreeIndex(uint32_t index) {
	// 使い終わったインデックスを再利用リストに積む
	freeIndices_.push(index);
}

D3D12_CPU_DESCRIPTOR_HANDLE MyEngine::LowLevel::BaseDescriptorHeapPool::GetCpuHandle(uint32_t index) const {
	assert(index < maxDescriptors_);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = heap_->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(index) * descriptorSize_;
	return handle;
}

uint32_t MyEngine::LowLevel::BaseDescriptorHeapPool::GetIndex(D3D12_CPU_DESCRIPTOR_HANDLE handle) const {
	D3D12_CPU_DESCRIPTOR_HANDLE startHandle = heap_->GetCPUDescriptorHandleForHeapStart();
	return static_cast<uint32_t>((handle.ptr - startHandle.ptr) / descriptorSize_);
}
