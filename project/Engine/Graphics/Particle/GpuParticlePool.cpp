#include "PCH.h"
#include "GpuParticlePool.h"
#include "ShaderManager.h"
#include "Function.h"
#include "SrvDescriptorHeapPool.h"
#include "PSOManager.h"
#include "InputLayoutManager.h"
#include "BlendModeManager.h"
#include "CameraOrganizer.h"
#include "GpuParticlePipelines.h"
#include "ParticleEffectData.h"

GpuParticlePool::GpuParticlePool(
	ID3D12Device* device,
	const GpuParticlePipelines& pipelines,    // System から借りる（Pool は消さない）
	const ParticleEffectData& data,
	uint32_t textureIndex,
	MyEngine::LowLevel::SrvDescriptorHeapPool& heap
) {
	assert(device != nullptr);

	// 引数を保持する
	pipelines_ = &pipelines;
	maxCount_ = data.maxCount;
	textureIndex_ = textureIndex;

	// UAVバッファ生成
	particleBuffer_ = CreateUAVBuffer(device, sizeof(GpuParticle) * maxCount_); // u0
	freeListBuffer_ = CreateUAVBuffer(device, sizeof(uint32_t) * maxCount_);    // u1
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

	// 定数バッファ
	emitterBuffer_.Initialize(device);
	emitterBuffer_.Update(emitter_);
	emitter_.maxCount = maxCount_;

	// バースト用のバッファ
	for(uint32_t i = 0; i < kMaxBurstPerFrame; ++i) {
		burstBuffers_[i].Initialize(device);
		burstBuffers_[i].Update(emitter_);
	}

	// 構造化バッファのSRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = maxCount_;
	srvDesc.Buffer.StructureByteStride = sizeof(GpuParticle);
	particleSrvIndex_ = heap.AllocateIndex();
	heap.CreateSRVforTexture2D(particleSrvIndex_, particleBuffer_.Get(), srvDesc); // 中身は CreateShaderResourceView なのでバッファにも使える

	// 描画用の定数バッファ
	drawBuffer_.Initialize(device);   // ConstantBuffer<ParticleDrawForGPU>

	// フラグ（作り直したときも、最初の Dispatch で必ず Initialize CS が走るように）
	needsInitialize_ = true;
	hasPendingCopy_ = false;
	freeListTop_ = -1;

	ApplyEffect(data);
}

void GpuParticlePool::Dispatch(
	ID3D12GraphicsCommandList* cmdList,
	D3D12_GPU_DESCRIPTOR_HANDLE srvTableStart,
	float deltaTime
) {
	ReadBackIfNeeded();

	cmdList->SetComputeRootSignature(pipelines_->computeRootSig);
	cmdList->SetComputeRootUnorderedAccessView(0, particleBuffer_->GetGPUVirtualAddress());
	cmdList->SetComputeRootUnorderedAccessView(1, freeListBuffer_->GetGPUVirtualAddress());
	cmdList->SetComputeRootUnorderedAccessView(2, freeListTopBuffer_->GetGPUVirtualAddress());
	cmdList->SetComputeRootConstantBufferView(3, emitterBuffer_.GetGPUVirtualAddress());
	cmdList->SetComputeRootDescriptorTable(4, srvTableStart);

	// Initialize CS の書き込みが終わってから Emit が読むようにする
	D3D12_RESOURCE_BARRIER uavBarrier{};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = nullptr; // nullptr = 全てのUAVに対して
	cmdList->ResourceBarrier(1, &uavBarrier);

	if(needsInitialize_) {
		cmdList->SetPipelineState(pipelines_->initPso);
		cmdList->Dispatch((maxCount_ + 255) / 256, 1, 1);
		needsInitialize_ = false;
		cmdList->ResourceBarrier(1, &uavBarrier);   // Init → Update
	}

	emitAccumulator_ += emitRate_ * deltaTime;
	uint32_t emitCount = static_cast<uint32_t>(emitAccumulator_);
	emitAccumulator_ -= static_cast<float>(emitCount);

	// このフレームの放出数と乱数の種
	emitter_.emitCount = emitCount;
	emitter_.deltaTime = deltaTime;
	emitter_.randomSeed = ++frameCount_ * 9781u; // 毎フレーム違う種にする（掛ける数は適当な大きめの素数）
	emitterBuffer_.Update(emitter_);

	// Update（全粒ぶん）
	cmdList->SetPipelineState(pipelines_->updatePso);
	cmdList->Dispatch((maxCount_ + 255) / 256, 1, 1);
	cmdList->ResourceBarrier(1, &uavBarrier);       // Update → Emit

	if(emitter_.emitCount > 0) {
		cmdList->SetPipelineState(pipelines_->emitPso);
		cmdList->Dispatch((emitter_.emitCount + 63) / 64, 1, 1);
	}

	if (!burstRequests_.empty()) {
		cmdList->SetPipelineState(pipelines_->emitPso);   // loop=false だと、まだ Update の PSO のままなので
	}

	for(uint32_t i = 0; i < burstRequests_.size(); ++i) {
		EmitterForGPU burst = emitter_;          // 見た目と動きの設定はそのまま
		burst.position = burstRequests_[i];
		burst.emitCount = burstCount_;
		burst.randomSeed = frameCount_ * 16 + i + 1;   // 同じフレームの Burst が同じ散り方にならないように
		burstBuffers_[i].Update(burst);

		// Slot3 = b0。今 emitterBuffer_ をセットしているのと同じ関数で
		cmdList->SetComputeRootConstantBufferView(3, burstBuffers_[i].GetGPUVirtualAddress());
		cmdList->Dispatch((burstCount_ + 63) / 64, 1, 1);
		cmdList->ResourceBarrier(1, &uavBarrier);   // 次の Emit も同じ空きリストを触るので
	}
	burstRequests_.clear();

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

void GpuParticlePool::Draw(
	ID3D12GraphicsCommandList* cmdList,
	ID3D12RootSignature* commonRootSig,
	D3D12_GPU_DESCRIPTOR_HANDLE srvTableStart,
	const CameraData& camera
) {
	// カメラの右・上ベクトル＝カメラのワールド行列の1行目・2行目
	ParticleDrawForGpu draw{};
	draw.viewProjection = camera.vp;
	draw.cameraRight = { camera.world.m[0][0], camera.world.m[0][1], camera.world.m[0][2] };
	draw.cameraUp = { camera.world.m[1][0], camera.world.m[1][1], camera.world.m[1][2] };
	draw.startColor = emitter_.startColor;
	draw.endColor = emitter_.endColor;
	draw.endSize = emitter_.endSize;
	drawBuffer_.Update(draw);

	// CS が書いた粒バッファを VS が読める状態にする
	MyEngine::Utility::TransitionBarrier(cmdList, particleBuffer_.Get(),
										 D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	cmdList->SetGraphicsRootSignature(commonRootSig);
	cmdList->SetPipelineState(drawPso_);
	cmdList->SetGraphicsRootConstantBufferView(0, drawBuffer_.GetGPUVirtualAddress()); // b0
	cmdList->SetGraphicsRoot32BitConstant(1, particleSrvIndex_, 0);                  // b1 の1つ目
	cmdList->SetGraphicsRootDescriptorTable(2, srvTableStart);                        // t0, space1
	cmdList->SetGraphicsRoot32BitConstant(1, textureIndex_, 1);       // b1 の2つ目
	cmdList->SetGraphicsRootDescriptorTable(3, srvTableStart);        // t0, space2（テクスチャ）
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(6, maxCount_, 0, 0);

	// 次のフレームの CS のために UAV に戻す
	MyEngine::Utility::TransitionBarrier(cmdList, particleBuffer_.Get(),
										 D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

void GpuParticlePool::ApplyEffect(const ParticleEffectData& data) {
	emitter_.shape = data.shape;
	emitter_.radius = data.radius;
	emitter_.boxSize = data.boxSize;
	emitter_.direction = data.direction;
	emitter_.spreadAngle = data.spreadAngle;   // CS がラジアンで使うなら、ここで変換する
	emitter_.speed = data.speed;
	emitter_.lifeTime = data.lifeTime;
	emitter_.acceleration = data.acceleration;
	emitter_.drag = data.drag;
	emitter_.startColor = data.startColor;
	emitter_.endColor = data.endColor;
	emitter_.startSize = data.startSize;
	emitter_.endSize = data.endSize;

	emitRate_ = data.loop ? data.rate : 0.0f;   // loop でないものは出し続けない
	burstCount_ = data.burstCount;
	drawPso_ = pipelines_->drawPso[static_cast<size_t>(data.blend)];
}

void GpuParticlePool::RequestBurst(const Vector3& position) {
	if(burstRequests_.size() >= kMaxBurstPerFrame) { return; }   // 溢れた分は捨てる
	burstRequests_.push_back(position);
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
	if(!hasPendingCopy_) return;

	// CPUから読む範囲（先頭から4バイト）
	D3D12_RANGE readRange{ 0, sizeof(int32_t) };
	void* mapped = nullptr;
	if(SUCCEEDED(readbackBuffer_->Map(0, &readRange, &mapped))) {
		std::memcpy(&freeListTop_, mapped, sizeof(int32_t));

		// CPUからは何も書いていないので、書いた範囲は 0〜0
		D3D12_RANGE writeRange{ 0, 0 };
		readbackBuffer_->Unmap(0, &writeRange);
	}
	hasPendingCopy_ = false;

	sDebugFreeListTop_ = freeListTop_; // ImGui 表示用（仮）
}