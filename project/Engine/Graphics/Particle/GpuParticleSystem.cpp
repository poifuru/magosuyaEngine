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
	// ---- パーティクル用のテクスチャを先に全部読んでおく ----
	// ゲーム中（ロード区間の外）は LoadTexture できないので、エディタはこの中から選ぶ
	const std::filesystem::path textureDir = "Resources/ParticleAssets";
	if (std::filesystem::exists(textureDir)) {
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(textureDir)) {
			if (entry.path().extension() != ".png") { continue; }

			// generic_string() で区切りを / に揃える（JSON に書くパスと文字列で一致させるため）
			const std::string path = entry.path().generic_string();
			textures_[path] = textureManager.LoadTexture(path);
			texturePaths_.push_back(path);
		}
	}

	// ---- エフェクトの JSON を読む ----
	const std::filesystem::path dir = "Resources/Particle";
	if (!std::filesystem::exists(dir)) { return; }

	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(dir)) {
		if (entry.path().extension() != ".json") { continue; }

		ParticleEffectData data;
		if (!LoadParticleEffect(entry.path().string(), data)) {
			continue;   // 1つ壊れていても他は読む（ログを出すと気づきやすい）
		}

		const std::string name = entry.path().stem().string();   // "fire.json" → "fire"
		// ParticleAssets の外の画像を使っているときだけ、ここで追加で読む
		if (textures_.count(data.texture) == 0) {
			textures_[data.texture] = textureManager.LoadTexture(data.texture);
			texturePaths_.push_back(data.texture);
		}
		uint32_t textureIndex = GetTextureIndex(data.texture);

		pools_[name] = std::make_unique<GpuParticlePool>(device_, pipelines_, data, textureIndex, *heap_);
		effects_[name] = data;
	}
}

void GpuParticleSystem::Emit(const std::string& name, const Vector3& position, uint32_t count) {
	GpuParticlePool* pool = FindPool(name);
	if (!pool) { return; }   // 名前の打ち間違い（ログを出すと気づきやすい）
	pool->RequestEmit(position, count);
}

void GpuParticleSystem::Burst(const std::string& name, const Vector3& position) {
	GpuParticlePool* pool = FindPool(name);
	if (!pool) { return; }
	pool->RequestEmit(position, pool->GetBurstCount());
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


std::vector<std::string> GpuParticleSystem::GetEffectNames() const {
	std::vector<std::string> names;
	for (const auto& [name, data] : effects_) {
		names.push_back(name);   // map なので名前順に並んでいる
	}
	return names;
}

ParticleEffectData* GpuParticleSystem::FindEffect(const std::string& name) {
	std::map<std::string, ParticleEffectData>::iterator it = effects_.find(name);
	if (it == effects_.end()) { return nullptr; }
	return &it->second;
}

void GpuParticleSystem::ApplyEffect(const std::string& name) {
	ParticleEffectData* data = FindEffect(name);
	GpuParticlePool* pool = FindPool(name);
	if (!data || !pool) { return; }

	uint32_t textureIndex = GetTextureIndex(data->texture);

	if (pool->GetMaxCount() != data->maxCount) {
		// バッファの大きさが変わった：Pool ごと作り直す（今飛んでいる粒は消える）
		// EndFrame で GPU の完了を待っているので、古い Pool を消しても GPU は使っていない
		// ※ SRV の番号は返せないので、作り直すたびに1つ使い捨てになる
		pools_[name] = std::make_unique<GpuParticlePool>(device_, pipelines_, *data, textureIndex, *heap_);
		return;
	}

	pool->ApplyEffect(*data);
	pool->SetTextureIndex(textureIndex);
}

bool GpuParticleSystem::SaveEffect(const std::string& name) const {
	std::map<std::string, ParticleEffectData>::const_iterator it = effects_.find(name);
	if (it == effects_.end()) { return false; }
	return SaveParticleEffect("Resources/Particle/" + name + ".json", it->second);
}

bool GpuParticleSystem::CreateEffect(const std::string& name) {
	if (name.empty()) { return false; }

	// ファイル名になるので半角英数字と _ だけにする（日本語は Windows のパスで文字化けするため）
	for (char c : name) {
		bool isAlphaNum = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
		if (!isAlphaNum && c != '_') { return false; }
	}

	// 同じ名前があれば作らない
	if (effects_.count(name) > 0) { return false; }

	ParticleEffectData data;   // デフォルト値
	uint32_t textureIndex = GetTextureIndex(data.texture);

	pools_[name] = std::make_unique<GpuParticlePool>(device_, pipelines_, data, textureIndex, *heap_);
	effects_[name] = data;

	// 作った時点でファイルも作っておく（作ったのに保存し忘れて消える、を防ぐ）
	return SaveEffect(name);
}

uint32_t GpuParticleSystem::GetTextureIndex(const std::string& path) const {
	std::map<std::string, uint32_t>::const_iterator it = textures_.find(path);
	if (it == textures_.end()) { return 0; }   // 読み込んでいないテクスチャ（0番を使う）
	return it->second;
}
