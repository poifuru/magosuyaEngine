#include "PCH.h"
#include "Game.h"
#include "Engine.h"
#include "Renderer.h"
#include "TitleScene.h"
#include "CameraOrganizer.h"
#include "ImGuiManager.h"
#include "RenderTexture.h"
#include "InputManager.h"
#include "WindowsAPI.h"
#include "SceneManager.h"

Game::Game() {
	engine_ = std::make_unique<MyEngine::LowLevel::Engine>();
	engine_->Initialize();

	renderer_ = std::make_unique<MyEngine::Rendering::Renderer>(
		engine_->GetDevice(),
		engine_->GetDxcUtils(),
		engine_->GetDxcCompiler(),
		engine_->GetIncludeHandler(),
		engine_->GetHeapPoolContext(),
		engine_->GetSwapChain()
	);

	// ロード用コマンドリストをリセットしてロード開始
	engine_->ResetCommandList();

	// カメラの初期化
	CameraOrganizer::GetInstance()->Initialize();

	// SceneManagerの生成と初期化
	sceneManager_ = std::make_unique<SceneManager>();
	sceneManager_->Initialize(
		engine_->GetGraphicsDevice(),
		engine_->GetCommandList(),
		engine_->GetSrvDescriptorHeapPool(),
		renderer_->GetRootSigManager(),
		renderer_->GetPSOManager(),
		renderer_->GetShaderManager(),
		renderer_->GetInputLayoutManager(),
		renderer_->GetBlendModeManager()
	);

	// 初期シーンの設定
	sceneManager_->ChangeScene<TitleScene>();
	sceneManager_->SetRenderer(renderer_.get());

	// ImGuiの初期化
	ImGuiManager::GetInstance()->Initialize(
		engine_->GetDevice(),
		engine_->GetCommandQueue(),
		engine_->GetSrvDescriptorHeapPool(),
		sceneManager_->GetTextureManager()
	);

	// コマンドリストを実行し、GPUのアップロード完了を待つ
	engine_->ExecuteCommandList();
}

Game::~Game() {
	ImGuiManager::GetInstance()->Finalize();
}

void Game::Run() {
	//ウィンドウの×ボタンが押されるまでループ
	while(true) {
 
 		if(engine_->ProcessMessage()) {
 			break;
 		}
 
 		// F11キーでボーダレスフルスクリーンのトグル切り替え
 		auto* rawInput = InputManager::GetInstance()->GetRawInput();
 		if (rawInput->Trigger(VK_F11)) {
 			auto* win = WindowsAPI::GetInstance();
 			// 現在フルスクリーンならウィンドウモードに、そうでないならボーダレスフルスクリーンにする
 			win->SetFullscreen(!win->IsFullscreen(), true); 
 		}
 
 		//フレーム開始
 		engine_->BeginFrame(renderer_->GetRenderTexture()->GetDescriptorHandle());

		// ImGui 新しいフレーム開始
		ImGuiManager::GetInstance()->BeginFrame(
			engine_->GetDevice(),
			engine_->GetSrvDescriptorHeapPool(),
			renderer_->GetFinalRenderTexture()
		);

		// --- Update (ゲーム更新・描画コマンド登録) の計測 ---
		auto startUpdate = std::chrono::high_resolution_clock::now();
		
		// シーンの更新
		sceneManager_->Update(&CameraOrganizer::GetInstance()->GetCameraData());
		// シーンの描画コマンドの積み込み
		sceneManager_->Draw(renderer_.get());
		
		auto endUpdate = std::chrono::high_resolution_clock::now();
		float updateMs = std::chrono::duration<float, std::milli>(endUpdate - startUpdate).count();
		MyEngine::LowLevel::Engine::SetUpdateTime(updateMs);

		// --- Render (描画実行・コマンドリスト構築) の計測 ---
		auto startRender = std::chrono::high_resolution_clock::now();
		
		// rendererで実際に描画
		renderer_->RenderScene(engine_->GetCommandList(), engine_->GetSrvDescriptorHeapPool());

		// SwapChainの切り替え(USEIMGUI時)
		engine_->BeginSwapChainRender();

		// ポストエフェクト
		if (auto* peManager = sceneManager_->GetPostEffectManager()) {
			renderer_->ExecutePostProcess(peManager);
		}

		// ImGuiの描画コマンド積み込み
		ImGuiManager::GetInstance()->Draw(engine_->GetCommandList());
		
		auto endRender = std::chrono::high_resolution_clock::now();
		float renderMs = std::chrono::duration<float, std::milli>(endRender - startRender).count();
		MyEngine::LowLevel::Engine::SetRenderTime(renderMs);

		// --- GPU Wait (EndFrameでのGPU同期待ち) の計測 ---
		auto startWait = std::chrono::high_resolution_clock::now();
		
		//フレーム終了
		engine_->EndFrame();
		
		auto endWait = std::chrono::high_resolution_clock::now();
		float waitMs = std::chrono::duration<float, std::milli>(endWait - startWait).count();
		MyEngine::LowLevel::Engine::SetGpuWaitTime(waitMs);
	}
}