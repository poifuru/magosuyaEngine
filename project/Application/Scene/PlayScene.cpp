#include "PCH.h"
#include "PlayScene.h"
#include "Renderer.h"
#include "RenderSystem.h"
#include "CameraOrganizer.h"
#include "EditorManager.h"
#include "ComponentType.h"
#include "InputManager.h"
#include "RawInput.h"
#include "CollisionManager.h"
#include "GraphicsDevice.h"
#include "LightManager.h"
#include "PostEffectManager.h"
#include "Dissolve.h"
#include "TextureManager.h"
#include "Dissolve.h"
#include "TitleScene.h"
#include "SceneManager.h"
#include "ScopeTimer.h"
#include "StartupSceneSettings.h"

namespace {
	// 安全にカーソルの表示/非表示を切り替える関数
	void SetCursorVisibleSafe(bool visible) {
		CURSORINFO ci = { sizeof(CURSORINFO) };
		if (GetCursorInfo(&ci)) {
			bool isCurrentlyVisible = (ci.flags & CURSOR_SHOWING) != 0;
			if (visible && !isCurrentlyVisible) {
				ShowCursor(TRUE);
			} else if (!visible && isCurrentlyVisible) {
				ShowCursor(FALSE);
			}
		}
	}
}

PlayScene::PlayScene() = default;

PlayScene::~PlayScene() {
#ifdef USEIMGUI
	if (EditorManager::GetInstance()) {
		EditorManager::GetInstance()->SetGamePaused(false);
	}
#endif
}

void PlayScene::Initialize() {
	if (!context_) return;

	isPaused_ = false;
#ifdef USEIMGUI
	if (EditorManager::GetInstance()) {
		EditorManager::GetInstance()->SetGamePaused(false);
	}
#endif

	// コンテキストにリストのポインタをセットする
	context_->gameObjects = &createQueue_;

	// 本番の生存リストをセット
	context_->activeGameObjects = &gameObjects_;

	// 起動シーン設定で指定されたシーンを読み込む（無ければ defaultScene.json）
	const std::string startupScenePath = StartupSceneSettings::LoadStartupScenePath();
	StartupSceneSettings::SetCurrentScenePath(startupScenePath);
	if (std::filesystem::exists(startupScenePath)) {
		std::ifstream file(startupScenePath);
		if (file.is_open()) {
			nlohmann::json sceneJ;
			{
				SCOPE_TIMER("シーンJSON読み込み");
				file >> sceneJ;
			}
			if (sceneJ.contains("objects")) {
				for (const auto& objJ : sceneJ["objects"]) {
					// オブジェクト1つごとの生成時間を計測
					SCOPE_TIMER("オブジェクト生成 " + objJ["name"].get<std::string>());
					auto newObj = std::make_unique<GameObject>(context_, objJ["name"]);
					newObj->Deserialize(objJ);
					newObj->Initialize();
					gameObjects_.push_back(std::move(newObj));
				}
				for (auto& obj : gameObjects_) {
					if (auto* followCam = obj->GetComponent<VirtualFollowCamera>()) {
						followCam->ResolveTarget(gameObjects_);
					}
				}
			}
		}
	}

	// --- カメラの初期優先度設定 ---
	bool isPlayingMode = true;
#ifdef USEIMGUI
	// エディタがある場合：Play中ならtrue、停止中ならfalse
	isPlayingMode = EditorManager::GetInstance()->IsPlaying();
#endif
	for (auto& obj : gameObjects_) {
		// 追従カメラ
		if (auto* followCam = obj->GetComponent<VirtualFollowCamera>()) {
			followCam->SetPriority(isPlayingMode ? 20 : 10);
		}
		// デバッグカメラ
		if (auto* debugCam = obj->GetComponent<VirtualDebugCamera>()) {
			debugCam->SetPriority(isPlayingMode ? 10 : 20);
		}
	}

	// ライトマネージャーの初期化
	{
		SCOPE_TIMER("LightManager初期化");
		lightManager_ = std::make_unique<LightManager>();
		lightManager_->Initialize(context_->graphicsDevice->GetDevice());
	}

	{
		SCOPE_TIMER("PostEffectManager初期化");
		postEffectManager_ = std::make_unique<PostEffectManager>();
		postEffectManager_->Initialize(context_->graphicsDevice->GetDevice());
	}

	// ディゾルブ用テクスチャをセット
	uint32_t noiseIndex = context_->textureManager->LoadTexture("Resources/noise0.png");
	if (auto* dissolve = postEffectManager_->GetEffect<Dissolve>(PostEffectType::Dissolve)) {
		dissolve->SetMaskTextureIndex(noiseIndex);
	}

	// EditorManagerにContextをセット
	EditorManager::GetInstance()->SetSceneContext(context_);
}

void PlayScene::TogglePause() {
	isPaused_ = !isPaused_;

#ifdef USEIMGUI
	if (EditorManager::GetInstance()) {
		EditorManager::GetInstance()->SetGamePaused(isPaused_);
	}
#endif

	if (isPaused_) {
		// 一時停止：マウスカーソルを表示し、画面クリップを解除
		SetCursorVisibleSafe(true);
		ClipCursor(NULL);
	} else {
		// ゲーム再開：ゲームはマウスを使わないので、カーソルは表示したまま・ロックもしない
		SetCursorVisibleSafe(true);
		ClipCursor(NULL);
	}
}

void PlayScene::DrawPauseMenu() {
#ifdef USEIMGUI
	// 画面中央にポーズメニューを表示
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(300.0f, 0.0f));

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | 
	                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;

	if (ImGui::Begin("一時停止 (PAUSE)", nullptr, flags)) {
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "         === PAUSED ===");
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (ImGui::Button("ゲームを再開 (Resume) [ESC]", ImVec2(-1, 35))) {
			TogglePause();
		}

		ImGui::Spacing();

		if (ImGui::Button("タイトルに戻る (Title)", ImVec2(-1, 35))) {
			TogglePause(); // ポーズ解除
			if (context_->sceneManager && !context_->sceneManager->isTransitioning()) {
				context_->sceneManager->ChangeSceneWithDissolve<TitleScene>(0.8f, 0.8f);
			}
		}

		ImGui::Spacing();
		ImGui::End();
	}
#endif
}

void PlayScene::UpdateGame(const CameraData* cameraData) {
	auto* rawInput = InputManager::GetInstance()->GetRawInput();

	// ESCキーで一時停止（ポーズ）/ 再開を切り替え
	if (rawInput->Trigger(VK_ESCAPE)) {
		TogglePause();
	}

	// 一時停止中の場合
	if (isPaused_) {
		// カーソル表示と画面ロック解除を維持
		SetCursorVisibleSafe(true);
		ClipCursor(NULL);

		// ポーズメニューを描画
		DrawPauseMenu();

		return; // ゲーム内の更新（移動、弾、当たり判定等）を完全に停止
	}

	for (auto& obj : gameObjects_) {
		obj->Update();
	}
	CollisionManager::GetInstance()->UpdateAllCollisions();

	// 追加待ちオブジェクトの合流や死亡削除など
	if (!createQueue_.empty()) {
		for (auto& newObj : createQueue_) {
			gameObjects_.push_back(std::move(newObj));
		}
		createQueue_.clear();
	}

	// 死亡オブジェクトの削除
	CleanupObject();

	// カメラ・ライト
	CameraOrganizer::GetInstance()->Update();
	if (lightManager_) {
		lightManager_->ClearLights();
		for (auto& obj : gameObjects_) {
			if (auto* light = obj->GetComponent<LightComponent>()) {
				lightManager_->Register(light);
			}
		}
		lightManager_->Update();
	}
}

void PlayScene::UpdateEdit(const CameraData* cameraData) {
	// カメラ（デバッグカメラ）を動かす
	for (auto& obj : gameObjects_) {
		if (auto* debugCam = obj->GetComponent<VirtualDebugCamera>()) {
			debugCam->Update(); // 停止中でもマウスでカメラを動かせるようにする
		}
	}
	CameraOrganizer::GetInstance()->Update();

	// ギズモで動かした座標を画面に反映させるため、Transformの行列バッファだけ更新する
	for (auto& obj : gameObjects_) {
		obj->UpdateTransformBuffer();
	}

	// ライトの更新
	if (lightManager_) {
		lightManager_->ClearLights();
		for (auto& obj : gameObjects_) {
			if (auto* light = obj->GetComponent<LightComponent>()) {
				lightManager_->Register(light);
			}
		}
		lightManager_->Update();
	}
}

void PlayScene::Draw(MyEngine::Rendering::Renderer* renderer) {
	// デバッグ線の描画要求（Draw は Play・Pause・Stop のどれでも毎フレーム呼ばれる）
	for (auto& obj : gameObjects_) {
		obj->DrawDebug();
	}

	// ライトマネージャーを RenderSystem に登録
	if (lightManager_ && renderer) {
		renderer->GetRenderSystem()->SetLightManager(lightManager_.get());
	}

	renderer->Draw(gameObjects_);

	renderer->SetPostEffectManager(postEffectManager_.get());
}

void PlayScene::CleanupObject() {
	// デスフラグ（isDead_）が立っているオブジェクトをリストから削除する
	for (auto it = gameObjects_.begin(); it != gameObjects_.end();) {
		if ((*it)->IsDead()) {
			it = gameObjects_.erase(it); // メモリ解放＆削除
		} else {
			++it;
		}
	}
}