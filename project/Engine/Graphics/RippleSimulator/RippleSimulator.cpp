#include "PCH.h"
#include "RippleSimulator.h"
#include "RootSignatureManager.h"

void RippleSimulator::Initialize(
	ID3D12Device* device,
	MyEngine::LowLevel::SrvDescriptorHeapPool& heapManager,
	MyEngine::Rendering::ShaderManager& shaderManager,
	MyEngine::Rendering::RootSignatureManager& rootSigManager,
	uint32_t width,
	uint32_t height
) {
	// パイプラインを初期化
	prePipeline_.Initialize(
		device,
		heapManager,
		shaderManager,
		L"Resources/Shader/Ripple.CS.hlsl",
		rootSigManager.GetComputeRootSignature(),
		width,
		height
	);

	currentPipeline_.Initialize(
		device,
		heapManager,
		shaderManager,
		L"Resources/Shader/Ripple.CS.hlsl",
		rootSigManager.GetComputeRootSignature(),
		width,
		height
	);
}

void RippleSimulator::Dispatch(
	ID3D12GraphicsCommandList* cmdList,
	const Vector3& boatPos,
	float speed, 
	float deltaTime,
	const Vector2& waterMin, 
	const Vector2& waterSize,
	const Vector2& forward,
	Vector4 kernel[43],
	int scrollX,
	int scrollY,
	const WaveSource* sources,
	uint32_t sourceCount
) {
	// 今回使うパイプライン (current) と 前回のパイプライン (prev)
	auto* currentPipeline = (currentIndex_ == 0) ? &prePipeline_ : &currentPipeline_;
	auto* prevPipeline    = (currentIndex_ == 0) ? &currentPipeline_ : &prePipeline_;

	// 前フレームのテクスチャを安全に読み込み状態 (ALL_SHADER_RESOURCE) に遷移
	prevPipeline->Transition(cmdList, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

	RippleSimulationParams params{};
	params.boatPosition = boatPos;
	params.time = deltaTime;
	params.textureSize = Vector2(512.0f, 512.0f);
	params.deltaTime = deltaTime;
	params.waterMin = waterMin;
	params.waterSize = waterSize;
	params.preTextureIndex = prevPipeline->GetTextureSrvIndex();
	params.speed = speed;
	params.forward = forward;
	for(int i = 0; i <= 42; ++i) {
		params.iWaveKernel[i] = kernel[i];
	}
	params.scrollX = scrollX;
	params.scrollY = scrollY;

	// 船以外の波源（入りきらない分は捨てる）
	params.sourceCount = (std::min)(sourceCount, RippleSimulationParams::kMaxWaveSources);
	for (uint32_t i = 0; i < params.sourceCount; ++i) {
		params.sources[i] = sources[i];
	}

	// 前フレームの船の位置 (初回は前フレームが無いので今の位置を使う)
	if (!hasPrevBoatPos_) {
		prevBoatPos_ = Vector2(boatPos.x, boatPos.z);
		hasPrevBoatPos_ = true;
	}
	params.prevBoatPosition = prevBoatPos_;

	// 現在のパイプラインのDispatchを呼ぶ
	currentPipeline->Dispatch(cmdList, params);

	// 水面へ渡す最新テクスチャのインデックスを更新
	textureIndex_ = currentPipeline->GetTextureSrvIndex();

	// 次回のために 0 と 1 を反転
	currentIndex_ = 1 - currentIndex_;

	// 次回のために今の船の位置を覚えておく
	prevBoatPos_ = Vector2(boatPos.x, boatPos.z);
}