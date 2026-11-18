#pragma once
#include "RenderingModel.h"

struct GpuParticlePipelines {
	ID3D12RootSignature* computeRootSig = nullptr;
	ID3D12PipelineState* initPso = nullptr;
	ID3D12PipelineState* emitPso = nullptr;
	ID3D12PipelineState* updatePso = nullptr;
	ID3D12PipelineState* drawPso[static_cast<size_t>(MyEngine::Rendering::BlendModeType::Count)] = {};
};