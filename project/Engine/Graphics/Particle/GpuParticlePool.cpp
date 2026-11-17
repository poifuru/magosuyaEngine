#include "PCH.h"
#include "GpuParticlePool.h"
#include "ShaderManager.h"
#include "Function.h"

void GpuParticlePool::Initialize(
	ID3D12Device* device,
	MyEngine::Rendering::ShaderManager& shaderManager,
	ID3D12RootSignature* particleComputeRootSig,
	uint32_t maxCount
) {
	assert(device != nullptr);
	assert(particleComputeRootSig != nullptr);
	assert(maxCount > 0);

	// 引数を保持する
	rootSig_ = particleComputeRootSig;
	maxCount_ = maxCount;
	
	// UAVバッファ生成
	particleBuffer_    = CreateUAVBuffer(device, sizeof(GpuParticle) * maxCount_); // u0
	freeListBuffer_    = CreateUAVBuffer(device, sizeof(uint32_t) * maxCount_);    // u1
	freeListTopBuffer_ = CreateUAVBuffer(device, sizeof(int32_t));    

	// リードバック用（頂上カウンタの4バイトだけ）
	D3D12_HEAP_PROPERTIES readbackHeap{};
	readbackHeap.Type = D3D12_HEAP_TYPE_READBACK;

	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = sizeof(int32_t);
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE; // READBACK には UAV フラグを付けない

	HRESULT hr = device->CreateCommittedResource(
		&readbackHeap, D3D12_HEAP_FLAG_NONE, &desc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
		IID_PPV_ARGS(readbackBuffer_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	// 定数バッファ（ステップ1では maxCount だけ使う）
	emitterBuffer_.Initialize(device);
	EmitterForGPU emitter{};
	emitter.maxCount = maxCount_;
	emitterBuffer_.Update(emitter);

	// Initialize CS の PSO
	uint32_t csID = shaderManager.CompileAndCacheShader(
		L"Resources/Shader/ParticleInit.CS.hlsl", L"cs_6_0");
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = rootSig_;
	psoDesc.CS = shaderManager.GetShaderBytecode(csID);
	hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(initPso_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	// フラグ（作り直したときも、最初の Dispatch で必ず Initialize CS が走るように）
	needsInitialize_ = true;
	hasPendingCopy_ = false;
	freeListTop_ = -1;
}

void GpuParticlePool::Dispatch(
	ID3D12GraphicsCommandList* cmdList,
	D3D12_GPU_DESCRIPTOR_HANDLE srvTableStart) {
	ReadBackIfNeeded();

	cmdList->SetComputeRootSignature(rootSig_);
	cmdList->SetComputeRootUnorderedAccessView(0, particleBuffer_->GetGPUVirtualAddress());
	cmdList->SetComputeRootUnorderedAccessView(1, freeListBuffer_->GetGPUVirtualAddress());
	cmdList->SetComputeRootUnorderedAccessView(2, freeListTopBuffer_->GetGPUVirtualAddress());
	cmdList->SetComputeRootConstantBufferView(3, emitterBuffer_.GetGPUVirtualAddress());
	// Slot4 は今は使わないが、テーブルは未設定のままにしない方が安全（heapManager->GetGpuHandle(0) を渡す）
	cmdList->SetComputeRootDescriptorTable(4, srvTableStart);

	if (needsInitialize_) {
		cmdList->SetPipelineState(initPso_.Get());
		cmdList->Dispatch((maxCount_ + 255) / 256, 1, 1);
		needsInitialize_ = false;
	}

	// 頂上カウンタをリードバックへコピー（UAV → コピー元 → コピー → UAVに戻す）
	MyEngine::Utility::TransitionBarrier(
		cmdList, 
		freeListTopBuffer_.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COPY_SOURCE
	);

	cmdList->CopyBufferRegion(
		readbackBuffer_.Get(),
		0,
		freeListTopBuffer_.Get(),
		0, 
		sizeof(int32_t)
		);

	MyEngine::Utility::TransitionBarrier(
		cmdList,
		freeListTopBuffer_.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);

	// 確認用：頂上カウンタをリードバックへコピー（WaterQuery と同じ遷移の仕方）
	// UAV → COPY_SOURCE → CopyBufferRegion(4バイト) → UAV
	hasPendingCopy_ = true;
}

int32_t GpuParticlePool::GetFreeListTop() {
	ReadBackIfNeeded(); // Dispatch より先に呼ばれても最新の値を返せるように

	return freeListTop_;
}

Microsoft::WRL::ComPtr<ID3D12Resource> GpuParticlePool::CreateUAVBuffer(ID3D12Device* device, UINT64 size) {
	Microsoft::WRL::ComPtr<ID3D12Resource> buffer = nullptr;

	D3D12_HEAP_PROPERTIES defaultHeap{};
	defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	HRESULT hr = device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &desc,
												 D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(buffer.GetAddressOf()));
	assert(SUCCEEDED(hr));

	return buffer;
}

void GpuParticlePool::ReadBackIfNeeded() {
	// コピーを頼んでいなければ何もしない
	if (!hasPendingCopy_) return;

	// CPUから読む範囲（先頭から4バイト）
	D3D12_RANGE readRange{ 0, sizeof(int32_t) };
	void* mapped = nullptr;
	if (SUCCEEDED(readbackBuffer_->Map(0, &readRange, &mapped))) {
		std::memcpy(&freeListTop_, mapped, sizeof(int32_t));

		// CPUからは何も書いていないので、書いた範囲は 0〜0
		D3D12_RANGE writeRange{ 0, 0 };
		readbackBuffer_->Unmap(0, &writeRange);
	}
	hasPendingCopy_ = false;

	sDebugFreeListTop_ = freeListTop_; // ImGui 表示用（仮）
}