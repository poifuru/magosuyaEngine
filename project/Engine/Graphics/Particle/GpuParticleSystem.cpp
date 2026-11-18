#include "PCH.h"
#include "GpuParticleSystem.h"
#include "PSOManager.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "ParticleEffectData.h"
#include "GpuParticlePool.h"

GpuParticleSystem::GpuParticleSystem(
	ID3D12Device* device,
	MyEngine::Rendering::ShaderManager& shaderManager,
	ID3D12RootSignature* particleComputeRootSig,
	MyEngine::Rendering::PSOManager& psoManager,
	const MyEngine::Rendering::InputLayoutManager& inputLayoutManager,
	const MyEngine::Rendering::BlendModeManager& blendModeManager,
	ID3D12RootSignature* commonRootSig,
	MyEngine::LowLevel::SrvDescriptorHeapPool& heap
) {
	device_ = device;
	heap_ = &heap;

	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = particleComputeRootSig;

	psoDesc.CS = shaderManager.GetShaderBytecode(
		shaderManager.CompileAndCacheShader(L"Resources/Shader/GpuParticleInit.CS.hlsl", L"cs_6_0"));
	HRESULT hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(initPso_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	psoDesc.CS = shaderManager.GetShaderBytecode(
		shaderManager.CompileAndCacheShader(L"Resources/Shader/GpuParticleEmitter.CS.hlsl", L"cs_6_0"));
	hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(emitPso_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	psoDesc.CS = shaderManager.GetShaderBytecode(
		shaderManager.CompileAndCacheShader(L"Resources/Shader/GpuParticleUpdate.CS.hlsl", L"cs_6_0"));
	hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(updatePso_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	MyEngine::Rendering::PSODescriptor drawPsoDesc{};
	drawPsoDesc.VS_ID = shaderManager.CompileAndCacheShader(L"Resources/Shader/GpuParticle.VS.hlsl", L"vs_6_0");
	drawPsoDesc.PS_ID = shaderManager.CompileAndCacheShader(L"Resources/Shader/GpuParticle.PS.hlsl", L"ps_6_0");
	drawPsoDesc.InputLayoutID = MyEngine::Rendering::InputLayoutType::PostProcess; // 入力レイアウト無し
	drawPsoDesc.CullMode = D3D12_CULL_MODE_NONE;
	drawPsoDesc.DepthEnable = TRUE;                            // 物の後ろには隠れる
	drawPsoDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;  // 粒同士では隠し合わない

	for (size_t i = 0; i < static_cast<size_t>(MyEngine::Rendering::BlendModeType::Count); ++i) {
		drawPsoDesc.BlendMode = static_cast<MyEngine::Rendering::BlendModeType>(i);
		pipelines_.drawPso[i] = psoManager.GetOrCreatePSO(
			device, drawPsoDesc, commonRootSig, shaderManager, inputLayoutManager, blendModeManager);
	}

	// pipelines_に詰める
	pipelines_.computeRootSig = particleComputeRootSig;
	pipelines_.initPso   = initPso_.Get();
	pipelines_.emitPso   = emitPso_.Get();
	pipelines_.updatePso = updatePso_.Get();
}

void GpuParticleSystem::LoadAll(TextureManager& textureManager) {
	const std::filesystem::path dir = "Resources/Particle";
	if (!std::filesystem::exists(dir)) { return; }

	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(dir)) {
		if (entry.path().extension() != ".json") { continue; }

		ParticleEffectData data;
		if (!LoadParticleEffect(entry.path().string(), data)) {
			continue;   // 1つ壊れていても他は読む（ログを出すと気づきやすい）
		}

		const std::string name = entry.path().stem().string();   // "fire.json" → "fire"
		uint32_t textureIndex = textureManager.LoadTexture(data.texture);

		pools_[name] = std::make_unique<GpuParticlePool>(device_, pipelines_, data, textureIndex, *heap_);
		effects_[name] = data;
	}
}

void GpuParticleSystem::Burst(const std::string& name, const Vector3& position) {
	std::map<std::string, std::unique_ptr<GpuParticlePool>>::iterator it = pools_.find(name);
	if (it == pools_.end()) { return; }   // 名前の打ち間違い ログを出すと気づきやすい
	it->second->RequestBurst(position);
}

void GpuParticleSystem::SetPosition(const std::string& name, const Vector3& position) {
	std::map<std::string, std::unique_ptr<GpuParticlePool>>::iterator it = pools_.find(name);
	if (it == pools_.end()) { return; }   // その名前のエフェクトが無い（打ち間違いなど）
	it->second->SetPosition(position);
}

void GpuParticleSystem::Dispatch(
	ID3D12GraphicsCommandList* cmdList,
	D3D12_GPU_DESCRIPTOR_HANDLE srvTableStart,
	float deltaTime
) {
	for (auto& [name, pool] : pools_) {
		pool->Dispatch(cmdList, srvTableStart, deltaTime);
	}
}

void GpuParticleSystem::Draw(
	ID3D12GraphicsCommandList* cmdList,
	ID3D12RootSignature* commonRootSig,
	D3D12_GPU_DESCRIPTOR_HANDLE srvTableStart,
	const CameraData& camera
) {
	for (auto& [name, pool] : pools_) {
		pool->Draw(cmdList, commonRootSig, srvTableStart, camera);
	}
}

GpuParticlePool* GpuParticleSystem::FindPool(const std::string& name) {
	std::map<std::string, std::unique_ptr<GpuParticlePool>>::iterator it = pools_.find(name);
	if (it == pools_.end()) { return nullptr; }   // その名前のエフェクトが無い
	return it->second.get();                      // unique_ptr の中身（生ポインタ）を返す。持ち主は System のまま
}
