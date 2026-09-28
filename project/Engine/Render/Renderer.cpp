#include "PCH.h"
#include "Renderer.h"
#include "DescriptorHeapManager.h"
#include "SwapChain.h"
#include "RootSignatureManager.h"
#include "WindowsAPI.h"
#include "ShaderManager.h"
#include "InputLayoutManager.h"
#include "BlendModeManager.h"
#include "PSOManager.h"
#include "RenderSystem.h"
#include "RenderTexture.h"
#include "RenderingModel.h"
#include "CameraOrganizer.h"
#include "Function.h"
#include "Model.h"
#include "Material.h"
#include "MeshData.h"
#include "ComponentType.h"
#include "PostEffectManager.h"
#include "Outline.h"

MyEngine::Rendering::Renderer::Renderer() = default;
MyEngine::Rendering::Renderer::~Renderer() = default;

void MyEngine::Rendering::Renderer::Initialize(
	ID3D12Device* device,
	IDxcUtils* dxcUtils,
	IDxcCompiler3* dxcCompiler,
	IDxcIncludeHandler* includeHandler,
	MyEngine::LowLevel::DescriptorHeapManager* heapManager,
	MyEngine::LowLevel::SwapChain* swapChain
) {
	device_ = device;
	heapManager_ = heapManager;
	swapChain_ = swapChain;

	rootSigManager_ = std::make_unique<MyEngine::Rendering::RootSignatureManager>();
	rootSigManager_->Initialize(device);

	shaderManager_ = std::make_unique<MyEngine::Rendering::ShaderManager>();
	shaderManager_->Initialize(
		dxcUtils,
		dxcCompiler,
		includeHandler
	);

	inputLayoutManager_ = std::make_unique<MyEngine::Rendering::InputLayoutManager>();
	inputLayoutManager_->Initialize();

	blendModeManager_ = std::make_unique<MyEngine::Rendering::BlendModeManager>();
	blendModeManager_->Initialize();

	psoManager_ = std::make_unique<MyEngine::Rendering::PSOManager>();

	renderSystem_ = std::make_unique<MyEngine::Rendering::RenderSystem>();
	renderSystem_->Initialize(
		device,
		heapManager,
		psoManager_.get(),
		shaderManager_.get(),
		inputLayoutManager_.get(),
		blendModeManager_.get(),
		rootSigManager_->GetCommonRootSignature()
	);

	renderTexture_ = std::make_unique<MyEngine::Rendering::RenderTexture>();
	renderTexture_->Initialize(device, heapManager);

	for(int i = 0; i < 2; ++i) {
		workTextures_[i] = std::make_unique<MyEngine::Rendering::RenderTexture>();
		workTextures_[i]->Initialize(device, heapManager);
	}

	InitializeShaderTable();
}

namespace {
	void SetupViewport(ID3D12GraphicsCommandList* cmdList) {
		D3D12_VIEWPORT viewport{};
		viewport.Width = static_cast<FLOAT>(WindowsAPI::GetInstance()->GetWindowWidth());
		viewport.Height = static_cast<FLOAT>(WindowsAPI::GetInstance()->GetWindowHeight());
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		cmdList->RSSetViewports(1, &viewport);
	}

	void SetupScissorRect(ID3D12GraphicsCommandList* cmdList) {
		D3D12_RECT scissorRect{};
		scissorRect.left = 0;
		scissorRect.top = 0;
		scissorRect.right = WindowsAPI::GetInstance()->GetWindowWidth();
		scissorRect.bottom = WindowsAPI::GetInstance()->GetWindowHeight();

		cmdList->RSSetScissorRects(1, &scissorRect);
	}
}

void MyEngine::Rendering::Renderer::RenderScene(
	ID3D12GraphicsCommandList* cmdList,
	MyEngine::LowLevel::DescriptorHeapManager* heapManager
) {
	cmdList_ = cmdList;

	// 描画する直前に RENDER_TARGET へバリア遷移
	renderTexture_->ChangeState(cmdList_, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// レンダーターゲットと深度バッファを renderTexture_ にセット
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = renderTexture_->GetDescriptorHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = swapChain_->GetDsvHandle();
	cmdList_->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// レンダーターゲットと深度バッファをクリア
	float clearColor[] = { 0.14f, 0.14f, 0.14f, 1.0f };
	cmdList_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	cmdList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	SetupViewport(cmdList_);
	SetupScissorRect(cmdList_);

	// 全コンポーネントのCSを一括実行
	DispatchCS(heapManager);

	// カメラ座標を取得して RenderSystem に設定
	Vector3 cameraPos = CameraOrganizer::GetInstance()->GetCameraData().transform.translate;
	renderSystem_->SetCameraPosition(cameraPos);

	renderSystem_->WriteCommandList(cmdList_);
	renderSystem_->ClearCommands();

	renderTexture_->ChangeState(cmdList_, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// カメラの Near/Far を Outline エフェクトに渡す
	auto cameraData = CameraOrganizer::GetInstance()->GetCameraData();
	if (postEffectManager_) {
		auto* outline = static_cast<Outline*>(postEffectManager_->GetEffect(static_cast<size_t>(PostEffectType::Outline)));
		if (outline) {
			outline->UpdateCameraNearFar(cameraData.nearClip, cameraData.farClip);
		}
	}
}

void MyEngine::Rendering::Renderer::DispatchCS(
	MyEngine::LowLevel::DescriptorHeapManager* heapManager
) {
	// CSでディスクリプタヒープを使う前にコマンドリストへセットする
	if(heapManager) {
		heapManager->SetGraphicsHeap(cmdList_);
	}

	if(currentObjects_) {
		for(auto& obj : *currentObjects_) {
			if(auto* wakeComp = obj->GetComponent<BoatWakeComponent>()) {
				wakeComp->DispatchCS(cmdList_);
			}
		}
	}
}

void MyEngine::Rendering::Renderer::Draw(std::vector<std::unique_ptr<GameObject>>& objects) {
	currentObjects_ = &objects; // 参照を保存

	for(auto& obj : objects) {

		// GameObjectの名前をあらかじめ取得しておく
		const char* objectName = obj->GetName().c_str();

		// 3Dモデルの描画
		if(auto* meshRenderer = obj->GetComponent<MeshRendererComponent>()) {
			// 非表示設定なら描画をスキップ
			if(meshRenderer->IsVisible() && !obj->GetComponent<WaterSurfaceComponent>()) {
				if(auto* model = meshRenderer->GetModel()) {
					auto* modelData = model->GetModelData();
					auto* material = model->GetMaterial();
					if(modelData && material) {
						for(size_t i = 0; i < modelData->meshes.size(); ++i) {
							auto transformAddr = model->GetMeshTransformGPUAddress(static_cast<uint32_t>(i));
							Submit(modelData->meshes[i], material, transformAddr, objectName);
						}
					}
				}
			}
		}
		// 2Dスプライトの描画
		if(auto* spriteComp = obj->GetComponent<SpriteComponent>()) {
			if(auto* model = spriteComp->GetModel()) {
				auto* modelData = model->GetModelData();
				auto* material = model->GetMaterial();
				auto transformAddr = model->GetTransformGPUAddress();
				if(modelData && material) {
					for(const auto& mesh : modelData->meshes) {
						Submit(mesh, material, transformAddr, objectName);
					}
				}
			}
		}
		// スカイボックスの描画
		if(auto* skybox = obj->GetComponent<SkyboxComponent>()) {
			Submit(
				skybox->GetMesh(),
				skybox->GetMaterial(),
				skybox->GetTransformAddress(),
				objectName
			);
		}
		// ナンバードローワーの描画 
		if(auto* numDrawer = obj->GetComponent<NumberDrawerComponent>()) {
			for(const auto& model : numDrawer->GetDigitModels()) {
				if(model) {
					auto* modelData = model->GetModelData();
					auto* material = model->GetMaterial();
					auto transformAddr = model->GetTransformGPUAddress();
					if(modelData && material) {
						for(const auto& mesh : modelData->meshes) {
							Submit(mesh, material, transformAddr, objectName);
						}
					}
				}
			}
		}
		// テキストドローワーの描画 
		if(auto* textDrawer = obj->GetComponent<TextDrawerComponent>()) {
			for(const auto& model : textDrawer->GetCharacterModels()) {
				if(model) {
					auto* modelData = model->GetModelData();
					auto* material = model->GetMaterial();
					auto transformAddr = model->GetTransformGPUAddress();
					if(modelData && material) {
						for(const auto& mesh : modelData->meshes) {
							Submit(mesh, material, transformAddr, objectName);
						}
					}
				}
			}
		}
		// 水面コンポーネントの描画
		if(auto* waterComp = obj->GetComponent<WaterSurfaceComponent>()) {
			if(auto* mesh  = waterComp->GetMesh()) {
				if(auto* material = waterComp->GetMaterial()) {
					Submit(
						*mesh, 
						material, 
						waterComp->GetTransformAddress(), 
						objectName, 
						waterComp->GetCustomBufferAddress()
					);
				}
			}
		}
		// BoatWakeComponent の描画
		if(auto* wakeComp = obj->GetComponent<BoatWakeComponent>()) {
			/*if (auto* material = wakeComp->GetMaterial()) {
				const auto& mesh = wakeComp->GetMesh();
				if (mesh.vertexCount > 0) {
					auto transformAddr = wakeComp->GetTransformAddress();
					Submit(mesh, material, transformAddr, objectName);
				}
			}*/

			OutputDebugStringA(">>> [1] Found BoatWakeComponent!\n");
			if(auto* material = wakeComp->GetMaterial()) {
				OutputDebugStringA(">>> [2] Material is OK!\n");
				const auto& mesh = wakeComp->GetMesh();
				char buf[256];
				sprintf_s(buf, ">>> [3] VertexCount = %u\n", mesh.vertexCount);
				OutputDebugStringA(buf);
				if(mesh.vertexCount > 0) {
					OutputDebugStringA(">>> [4] Submitting BoatWake to RenderSystem!\n");
					auto transformAddr = wakeComp->GetTransformAddress();
					Submit(mesh, material, transformAddr, objectName);
				}
			}
			else {
				OutputDebugStringA(">>> [ERR] Material is NULL!\n");
			}
		}
	}
}

void MyEngine::Rendering::Renderer::ExecutePostProcess(
	PostEffectManager* postEffectManager
) {
	if(!postEffectManager) return;

	renderTexture_->ChangeState(cmdList_, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// ピンポンレンダリング
	Pingpong(postEffectManager);

	// CopyImageをワークバッファに描画して finalRenderTexture_ を更新する
	RenderTexture* toneMapOutput = workTextures_[0].get();
	if (finalRenderTexture_ == workTextures_[0].get()) {
		toneMapOutput = workTextures_[1].get(); // 被らない方のバッファを使う
	}

	toneMapOutput->ChangeState(cmdList_, D3D12_RESOURCE_STATE_RENDER_TARGET);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = toneMapOutput->GetDescriptorHandle();
	cmdList_->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// ワークバッファへの描画なのでlastEffectはfalse
	lastEffect_ = false;

	// トーンマップ実行（CopyImage.PS.hlsl）
	SubmitPostEffect(
		cmdList_,
		ShadingModel::PostEffect_CopyImage,
		0,
		finalRenderTexture_->GetSrvIndex(),
		0
	);
	toneMapOutput->ChangeState(cmdList_, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// ここで最終結果をトーンマップ済みのテクスチャに差し替える
	finalRenderTexture_ = toneMapOutput;

	// バックバッファへの書き込み時だけlastEffectをtrue
	lastEffect_ = true;
	// 画面（バックバッファ）にも描画しておく
	SetBackBufferAsRenderTarget();
	lastEffect_ = true;
	SubmitPostEffect(
		cmdList_,
		ShadingModel::PostEffect_CopyImage,
		0,
		finalRenderTexture_->GetSrvIndex(),
		0
	);
	lastEffect_ = false;
}

uint64_t MyEngine::Rendering::Renderer::MakeShaderKey(
	MyEngine::Rendering::ShadingModel model,
	MyEngine::Rendering::InputLayoutType layout
) {
	// 上位32bitにShadingModel、下位32bitにInputLayoutTypeを詰める
	return (static_cast<uint64_t>(model) << 32) | static_cast<uint64_t>(layout);
}

void MyEngine::Rendering::Renderer::InitializeShaderTable() {
	// シェーダーテーブルを構築する
	auto* sm = shaderManager_.get();

	// Standard x Standard3D
	uint32_t stdVS = sm->CompileAndCacheShader(L"Resources/Shader/Object3d.VS.hlsl", L"vs_6_0");
	uint32_t stdPS = sm->CompileAndCacheShader(L"Resources/Shader/Object3d.PS.hlsl", L"ps_6_0");
	shaderTable_[MakeShaderKey(ShadingModel::Standard, InputLayoutType::Standard3D)] = { stdVS, stdPS };

	// Skybox x Skybox
	uint32_t skyVS = sm->CompileAndCacheShader(L"Resources/Shader/Skybox.VS.hlsl", L"vs_6_0");
	uint32_t skyPS = sm->CompileAndCacheShader(L"Resources/Shader/Skybox.PS.hlsl", L"ps_6_0");
	shaderTable_[MakeShaderKey(ShadingModel::Skybox, InputLayoutType::Skybox)] = { skyVS, skyPS };

	// WarterSurface x Standard3D
	uint32_t watVS = sm->CompileAndCacheShader(L"Resources/Shader/WaterSurface.VS.hlsl", L"vs_6_0");
	uint32_t watPS = sm->CompileAndCacheShader(L"Resources/Shader/WaterSurface.PS.hlsl", L"ps_6_0");
	shaderTable_[MakeShaderKey(ShadingModel::WaterSurface, InputLayoutType::Standard3D)] = { watVS, watPS };

	// BoatWake x BoatWake
	uint32_t wakeVS = sm->CompileAndCacheShader(L"Resources/Shader/BoatWake.VS.hlsl", L"vs_6_0");
	uint32_t wakePS = sm->CompileAndCacheShader(L"Resources/Shader/BoatWake.PS.hlsl", L"ps_6_0");
	shaderTable_[MakeShaderKey(ShadingModel::BoatWake, InputLayoutType::BoatWake)] = { wakeVS, wakePS };

	// OutlineObject x Standard3D
	uint32_t outObjVS = sm->CompileAndCacheShader(L"Resources/Shader/OutlineObject.VS.hlsl", L"vs_6_0");
	uint32_t outObjPS = sm->CompileAndCacheShader(L"Resources/Shader/OutlineObject.PS.hlsl", L"ps_6_0");
	shaderTable_[MakeShaderKey(ShadingModel::OutlineObject, InputLayoutType::Standard3D)] = { outObjVS, outObjPS };

	//*** PostEffect ***//
	// VSは共通
	uint32_t postEffectVS = sm->CompileAndCacheShader(L"Resources/shader/Fullscreen.VS.hlsl", L"vs_6_0");

	// CopyImage
	uint32_t copyImage = sm->CompileAndCacheShader(L"Resources/shader/CopyImage.PS.hlsl", L"ps_6_0");
	shaderTable_[MakeShaderKey(ShadingModel::PostEffect_CopyImage, InputLayoutType::PostProcess)] = { postEffectVS, copyImage };

	// Outline
	uint32_t outline = sm->CompileAndCacheShader(L"Resources/shader/Outline.PS.hlsl", L"ps_6_0");
	shaderTable_[MakeShaderKey(ShadingModel::PostEffect_Outline, InputLayoutType::PostProcess)] = { postEffectVS, outline };

	// ColorGrading
	uint32_t colorGrading = sm->CompileAndCacheShader(L"Resources/shader/ColorGrading.PS.hlsl", L"ps_6_0");
	shaderTable_[MakeShaderKey(ShadingModel::PostEffect_ColorGrading, InputLayoutType::PostProcess)] = { postEffectVS, colorGrading };

	// Fog
	uint32_t fog = sm->CompileAndCacheShader(L"Resources/shader/Fog.PS.hlsl", L"ps_6_0");
	shaderTable_[MakeShaderKey(ShadingModel::PostEffect_Fog, InputLayoutType::PostProcess)] = { postEffectVS, fog };

	// Vignette
	uint32_t vignette = sm->CompileAndCacheShader(L"Resources/shader/Vignette.PS.hlsl", L"ps_6_0");
	shaderTable_[MakeShaderKey(ShadingModel::PostEffect_Vignette, InputLayoutType::PostProcess)] = { postEffectVS, vignette };

	// RadialBlur
	uint32_t radialBlur = sm->CompileAndCacheShader(L"Resources/shader/RadialBlur.PS.hlsl", L"ps_6_0");
	shaderTable_[MakeShaderKey(ShadingModel::PostEffect_RadialBlur, InputLayoutType::PostProcess)] = { postEffectVS, radialBlur };

	// Dissolve
	uint32_t dissolve = sm->CompileAndCacheShader(L"Resources/shader/Dissolve.PS.hlsl", L"ps_6_0");
	shaderTable_[MakeShaderKey(ShadingModel::PostEffect_Dissolve, InputLayoutType::PostProcess)] = { postEffectVS, dissolve };

	// RandomNoise
	uint32_t randomNoise = sm->CompileAndCacheShader(L"Resources/shader/RandomNoise.PS.hlsl", L"ps_6_0");
	shaderTable_[MakeShaderKey(ShadingModel::PostEffect_RandomNoise, InputLayoutType::PostProcess)] = { postEffectVS, randomNoise };
}

void MyEngine::Rendering::Renderer::Submit(
	const MyEngine::Rendering::Mesh& mesh,
	MyEngine::Rendering::Material* material,
	D3D12_GPU_VIRTUAL_ADDRESS transformAddr,
	const char* debugName,
	D3D12_GPU_VIRTUAL_ADDRESS customAddr
) {
	if(!material) return;

	auto key = MakeShaderKey(material->GetShadingModel(), mesh.inputLayout);
	auto it = shaderTable_.find(key);
	assert(it != shaderTable_.end() && "シェーダーが未登録です");
	const ShaderPair& shaders = it->second;

	// PSODescriptorを組み立てる
	PSODescriptor desc{};
	desc.VS_ID = shaders.vs_ID;
	desc.PS_ID = shaders.ps_ID;
	desc.InputLayoutID = mesh.inputLayout;
	desc.BlendMode = material->GetBlendMode();
	desc.DepthEnable = material->IsDepthEnable();

	desc.DepthWriteMask = material->IsDepthWrite()
		? D3D12_DEPTH_WRITE_MASK_ALL
		: D3D12_DEPTH_WRITE_MASK_ZERO;

	if (material->GetShadingModel() == ShadingModel::OutlineObject) {
		desc.CullMode = D3D12_CULL_MODE_FRONT;
	} else {
		desc.CullMode = material->IsDoubleSided()
			? D3D12_CULL_MODE_NONE
			: D3D12_CULL_MODE_BACK;
	}

	// PSOManagerに渡してPSOを取得（なければ生成・キャッシュ）
	ID3D12PipelineState* pso = psoManager_->GetOrCreatePSO(
		device_,
		desc,
		rootSigManager_->GetCommonRootSignature(),
		*shaderManager_,
		*inputLayoutManager_,
		*blendModeManager_
	);

	// RenderCommandに詰めてキューに積む
	RenderCommand cmd{};
	cmd.pso = pso;
	cmd.vbView = mesh.vbView;
	cmd.ibv = mesh.ibView;
	cmd.vertexCount = mesh.vertexCount;
	cmd.indexCount = mesh.indexCount;
	cmd.transformGPUAddress = transformAddr;
	cmd.customBufferGPUAddress = customAddr;
	cmd.materialIndex = material->GetDescriptorIndex();
	cmd.textureIndex = material->GetTextureIndex();
	cmd.layer = material->GetLayer();
	cmd.debugName = debugName;
	renderSystem_->PushCommand(cmd);
}

void MyEngine::Rendering::Renderer::SubmitPostEffect(
	ID3D12GraphicsCommandList* cmdList,
	MyEngine::Rendering::ShadingModel model,
	D3D12_GPU_VIRTUAL_ADDRESS cbvAddress,
	uint32_t srcTextureSrvIndex,
	uint32_t extraSrvIndex
) {
	// シェーダーキーを取得
	auto key = MakeShaderKey(model, MyEngine::Rendering::InputLayoutType::PostProcess);
	auto it = shaderTable_.find(key);
	assert(it != shaderTable_.end() && "シェーダーが未登録です");
	const ShaderPair& shaders = it->second;

	// PSODescriptorを組み立てる
	PSODescriptor desc{};
	desc.VS_ID = shaders.vs_ID;
	desc.PS_ID = shaders.ps_ID;
	desc.BlendMode = MyEngine::Rendering::BlendModeType::Opaque;

	desc.CullMode = D3D12_CULL_MODE_NONE; // カリングなし
	desc.FillMode = D3D12_FILL_MODE_SOLID;
	desc.DepthEnable = FALSE;

	if(lastEffect_) {
		desc.RTVFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	}

	ID3D12PipelineState* pso = psoManager_->GetOrCreatePSO(
		device_, desc, rootSigManager_->GetCommonRootSignature(),
		*shaderManager_, *inputLayoutManager_, *blendModeManager_
	);

	// ルートシグネチャとPSOをバインド
	cmdList_->SetGraphicsRootSignature(rootSigManager_->GetCommonRootSignature());
	cmdList_->SetPipelineState(pso);
	cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 定数バッファ (b0) のバインド
	if(cbvAddress != 0) {
		cmdList_->SetGraphicsRootConstantBufferView(0, cbvAddress);
	}

	// テクスチャ SRV (t0: 入力画面, t1: ディゾルブ用マスク等) のバインド
	D3D12_GPU_DESCRIPTOR_HANDLE texHandle = heapManager_->GetGpuHandle(srcTextureSrvIndex);
	D3D12_GPU_DESCRIPTOR_HANDLE extraTexHandle = heapManager_->GetGpuHandle(extraSrvIndex);

	cmdList_->SetGraphicsRootDescriptorTable(3, texHandle);
	if(extraSrvIndex != 0) {
		cmdList_->SetGraphicsRootDescriptorTable(2, extraTexHandle);
	}

	// 全画面ポリゴンの描画
	cmdList_->DrawInstanced(3, 1, 0, 0);
}

void MyEngine::Rendering::Renderer::Pingpong(PostEffectManager* postEffectManager) {
	// フラグのリセット
	lastEffect_ = false;

	// 最後に描画するアクティブなエフェクトのインデックスを探す
	int lastActiveIndex = -1;
	for(int i = static_cast<int>(postEffectManager->GetEffectCount()) - 1; i >= 0; --i) {
		auto* effect = postEffectManager->GetEffect(i);
		if(effect && effect->GetIsActive()) {
			lastActiveIndex = i;
			break;
		}
	}

	// アクティブなエフェクトが一つも無ければ、素の renderTexture_ を最終結果にして終了
	if(lastActiveIndex == -1) {
		lastEffect_ = true;
		finalRenderTexture_ = renderTexture_.get();
		return;
	}

	// ピンポンレンダリングのループ開始
	RenderTexture* currentInput = renderTexture_.get(); // 最初はメインシーンのテクスチャ
	int currentTargetIndex = 0;

	for(size_t i = 0; i < postEffectManager->GetEffectCount(); ++i) {
		auto* effect = postEffectManager->GetEffect(i);
		if(!effect || !effect->GetIsActive()) continue;

		// 出力先を常に中間バッファ workTextures_[currentTargetIndex] に設定
		RenderTexture* nextOutput = workTextures_[currentTargetIndex].get();

		// RENDER_TARGET 状態へバリア遷移
		nextOutput->ChangeState(cmdList_, D3D12_RESOURCE_STATE_RENDER_TARGET);

		// OMSetRenderTargets & Clear
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = nextOutput->GetDescriptorHandle();
		cmdList_->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
		float clearColor[4] = { 0.14f, 0.14f, 0.14f, 1.0f };
		cmdList_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		// 深度が必要なエフェクトかチェック
		bool isDepthEffect = (effect->GetShadingModel() == ShadingModel::PostEffect_Outline ||
							  effect->GetShadingModel() == ShadingModel::PostEffect_Fog);

		if (isDepthEffect) {
			// 深度バッファを DEPTH_WRITE -> PIXEL_SHADER_RESOURCE へ遷移
			MyEngine::Utility::TransitionBarrier(
				cmdList_,
				swapChain_->GetDepthBufferResource(),
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			);
		}

		// 描画実行
		SubmitPostEffect(
			cmdList_,
			effect->GetShadingModel(),
			effect->GetConstantBufferAddress(),
			currentInput->GetSrvIndex(), // 入力テクスチャ
			effect->GetExtraSrvIndex()   // 追加テクスチャ(マスク等)
		);

		if (isDepthEffect) {
			// 深度バッファを PIXEL_SHADER_RESOURCE -> DEPTH_WRITE へ戻す
			MyEngine::Utility::TransitionBarrier(
				cmdList_,
				swapChain_->GetDepthBufferResource(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_DEPTH_WRITE
			);
		}

		// 今書き込んだバッファを SRV (読み込み用) にバリア遷移
		nextOutput->ChangeState(cmdList_, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		// 次のエフェクトのために入力ソースを更新し、ターゲットインデックスをフリップ (0 ⇐⇒ 1)
		currentInput = nextOutput;
		currentTargetIndex = 1 - currentTargetIndex;
	}

	// 最終的に描画された結果のテクスチャを保存
	finalRenderTexture_ = currentInput;
}

void MyEngine::Rendering::Renderer::SetBackBufferAsRenderTarget() {
	// 現在のバックバッファインデックスとRTVハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = swapChain_->GetCurrentBackBufferRtvHandle();

	// レンダーターゲットに設定 (バックバッファへ描画)
	cmdList_->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
}