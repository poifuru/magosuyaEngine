#pragma once
#include "SrvDescriptorHeap.h"

// 構造化バッファのクラステンプレート
template <typename T>
class StructuredBuffer {
public:
	StructuredBuffer() = default;

	~StructuredBuffer() { Release(); }

	// 要素数(count)を指定して初期化
	void Initialize(ID3D12Device* device, MyEngine::LowLevel::SrvDescriptorHeap& heapManager, size_t count) {
		Release();
		assert(device != nullptr);

		heapManager_ = &heapManager;
		elementCount_ = count;
		size_t bufferSize = sizeof(T) * elementCount_;

		D3D12_HEAP_PROPERTIES uploadHeapProperties = {};
		uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = bufferSize;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		HRESULT hr = device->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(buffer_.GetAddressOf())
		);
		assert(SUCCEEDED(hr));

		hr = buffer_->Map(0, nullptr, &mappedData_);
		assert(SUCCEEDED(hr));

		// バインドレスヒープからインデックスを1つ切り出す
		descriptorIndex_ = heapManager_->AllocateIndex();

		// 構造化バッファとしての設定を組み立てる
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		srvDesc.Buffer.NumElements = static_cast<UINT>(elementCount_);
		srvDesc.Buffer.StructureByteStride = static_cast<UINT>(sizeof(T));

		// SrvDescriptorHeapで、このインデックス位置へSRVを焼き付ける
		heapManager_->CreateSRVforTexture2D(descriptorIndex_, buffer_.Get(), srvDesc);
	}

	void Update(const std::vector<T>& data) {
		if (!mappedData_ || data.empty()) return;
		size_t copyCount = (std::min)(elementCount_, data.size());
		std::memcpy(mappedData_, data.data(), sizeof(T) * copyCount);
	}

	void Release() {
		if (buffer_) {
			if (mappedData_) {
				buffer_->Unmap(0, nullptr);
			}
			// 使い終わったら、アロケーションされていたインデックスを自動で返却する
			if (heapManager_) {
				heapManager_->FreeIndex(descriptorIndex_);
			}
		}
		buffer_ = nullptr;
		mappedData_ = nullptr;
		elementCount_ = 0;
		descriptorIndex_ = 0;
		heapManager_ = nullptr;
	}

	// アクセッサ
	uint32_t GetDescriptorIndex() const { return descriptorIndex_; }
	ID3D12Resource* GetResource() const { return buffer_.Get(); }
	size_t GetElementCount() const { return elementCount_; }

public:
	// コピー禁止（ポインタの二重管理を防ぐため）
	StructuredBuffer(const StructuredBuffer&) = delete;
	StructuredBuffer& operator=(const StructuredBuffer&) = delete;

	// ムーブは許可
	StructuredBuffer(StructuredBuffer&& other) noexcept { *this = std::move(other); }
	StructuredBuffer& operator=(StructuredBuffer&& other) noexcept {
		if (this != &other) {
			Release();
			buffer_ = std::move(other.buffer_);
			mappedData_ = other.mappedData_;
			elementCount_ = other.elementCount_;
			descriptorIndex_ = other.descriptorIndex_;
			heapManager_ = other.heapManager_;

			other.buffer_ = nullptr;
			other.mappedData_ = nullptr;
			other.elementCount_ = 0;
			other.descriptorIndex_ = 0;
			other.heapManager_ = nullptr;
		}
		return *this;
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> buffer_;
	void* mappedData_ = nullptr;
	size_t elementCount_ = 0;

	uint32_t descriptorIndex_ = 0;
	MyEngine::LowLevel::SrvDescriptorHeap* heapManager_ = nullptr;
};