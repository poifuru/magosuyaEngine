#include "PCH.h"
#include "EditorManager.h"
#include "RenderTexture.h"
#include "SrvDescriptorHeapPool.h"
#include "CommandManager.h"
#include "BaseScene.h"
#include "GameObject.h"
#include "VirtualDebugCamera.h"
#include "VirtualFollowCamera.h"
#include "PlayerComponent.h"
#include "CameraOrganizer.h"

// 各ウィンドウクラス
#include "PerformanceWindow.h"
#include "GameViewWindow.h"
#include "AssetBrowserWindow.h"
#include "HierarchyWindow.h"
#include "InspectorWindow.h"
#include "GizmoWindow.h"

namespace {
	// 現在の表示状態をチェックして安全に表示/非表示を切り替える関数
	void SetCursorVisible(bool visible) {
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

	// マウスをゲーム画面の枠内に閉じ込める（または解除する）関数
	void SetMouseClip(bool enable) {
		if (enable) {
			ImVec2 pos = EditorManager::GetInstance()->GetGameScreenPos();
			ImVec2 size = EditorManager::GetInstance()->GetGameScreenSize();
			if (size.x > 0.0f && size.y > 0.0f) {
				RECT rect;
				rect.left = static_cast<LONG>(pos.x);
				rect.top = static_cast<LONG>(pos.y);
				rect.right = static_cast<LONG>(pos.x + size.x);
				rect.bottom = static_cast<LONG>(pos.y + size.y);
				ClipCursor(&rect);
			}
		} else {
			ClipCursor(NULL);
		}
	}
}

void EditorManager::Initialize() {
	// ウィンドウを登録
	RegisterWindow<PerformanceWindow>();
	RegisterWindow<GameViewWindow>();
	RegisterWindow<AssetBrowserWindow>();
	RegisterWindow<HierarchyWindow>();
	RegisterWindow<InspectorWindow>();
	RegisterWindow<GizmoWindow>();

	// 前回の開閉状態を復元
	LoadLayoutSettings();
}

void EditorManager::Finalize() {
	// 終了時に状態を保存
	SaveLayoutSettings();

	// マウスロックとカーソルを確実に解除
	SetMouseClip(false);
	SetCursorVisible(true);
}

void EditorManager::UpdateAndDraw(
	ID3D12Device* device,
	MyEngine::LowLevel::SrvDescriptorHeapPool* srvHeap,
	MyEngine::Rendering::RenderTexture* renderTexture
) {
#ifdef USEIMGUI
	// Ctrl + Z で Undo (元に戻す)
	if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) {
		CommandManager::GetInstance()->Undo();
	}
	// Ctrl + Y で Redo (やり直す)
	if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) {
		CommandManager::GetInstance()->Redo();
	}

	//*** 各ウィンドウを順番に描画していく ***//
	// メニューバー
	DrawMenuBar();

	// EditorContextを詰めて、登録された各ウィンドウを描画
	EditorContext context{};
	context.device = device;
	context.srvHeap = srvHeap;
	context.renderTexture = renderTexture;
	context.sceneContext = sceneContext_;
	context.selectedObject = &selectedObject_;
	for (auto& window : windows_) {
		window->UpdateAndDraw(context);
	}
#endif
}

void EditorManager::LoadAssetBrowserIcon(TextureManager* texManager) {
	GetWindow<AssetBrowserWindow>()->LoadIconTexture(texManager);
}

bool EditorManager::IsGameWindowHovered() const {
	if (auto* w = GetWindow<GameViewWindow>()) return w->IsHovered();
	return false;
}

bool EditorManager::IsGameWindowFocused() const {
	if (auto* w = GetWindow<GameViewWindow>()) return w->IsFocused();
	return false;
}

ImVec2 EditorManager::GetGameScreenPos() const {
	if (auto* w = GetWindow<GameViewWindow>()) return w->GetGameScreenPos();
	return { 0.0f, 0.0f };
}

ImVec2 EditorManager::GetGameScreenSize() const {
	if (auto* w = GetWindow<GameViewWindow>()) return w->GetGameScreenSize();
	return { 0.0f, 0.0f };
}

void EditorManager::SetGizmoActive(bool active) {
	if (auto* w = GetWindow<GameViewWindow>()) w->SetGizmoActive(active);
}

bool EditorManager::IsGizmoActive() const {
	if (auto* w = GetWindow<GameViewWindow>()) return w->IsGizmoActive();
	return false;
}

void EditorManager::SaveLayoutSettings() {
	nlohmann::json root;
	root["ウィンドウ"] = nlohmann::json::object();
	for (auto& window : windows_) {
		root["ウィンドウ"][window->GetName()] = window->IsOpen();
	}
	std::filesystem::path path(settingsFilePath_);
	if (path.has_parent_path() && !std::filesystem::exists(path.parent_path())) {
		std::filesystem::create_directories(path.parent_path());
	}
	std::ofstream file(settingsFilePath_);
	if (file.is_open()) {
		file << root.dump(4);
	}
}

void EditorManager::LoadLayoutSettings() {
	if (!std::filesystem::exists(settingsFilePath_)) {
		return;
	}
	std::ifstream file(settingsFilePath_);
	if (!file.is_open()) {
		return;
	}
	nlohmann::json root;
	file >> root;
	if (root.contains("ウィンドウ") && root["ウィンドウ"].is_object()) {
		for (auto& window : windows_) {
			const std::string& name = window->GetName();
			if (root["ウィンドウ"].contains(name)) {
				window->SetOpen(root["ウィンドウ"][name].get<bool>());
			}
		}
	}
}

void EditorManager::Play() {
	if (playState_ == EditorPlayState::Edit) {
		// 再生前のシーン状態をスナップショットとしてメモリに保存
		sceneSnapshot_.clear();
		if (sceneContext_ && sceneContext_->activeGameObjects) {
			sceneSnapshot_["objects"] = nlohmann::json::array();
			for (auto& obj : *sceneContext_->activeGameObjects) {
				if (obj->IsSerializable()) {
					sceneSnapshot_["objects"].push_back(obj->Serialize());
				}
			}
		}
	}
	playState_ = EditorPlayState::Play;

	// ゲームプレイ用カメラ（追従カメラ）を優先にする
	if (sceneContext_ && sceneContext_->activeGameObjects) {
		for (auto& obj : *sceneContext_->activeGameObjects) {
			if (auto* followCam = obj->GetComponent<VirtualFollowCamera>()) {
				followCam->SetPriority(20); // 追従カメラを高優先度
			}
			if (auto* debugCam = obj->GetComponent<VirtualDebugCamera>()) {
				debugCam->SetPriority(10);  // デバッグカメラを低優先度
			}
		}
	}

	// プレイ開始：マウスカーソルを消してゲーム画面にロック
	SetCursorVisible(false);
	SetMouseClip(true);
}

void EditorManager::Pause() {
	if (playState_ == EditorPlayState::Play) {
		playState_ = EditorPlayState::Pause;
		// 一時停止中：デバッグカメラで自由に周囲を見回せるように優先度を切り替える
		if (sceneContext_ && sceneContext_->activeGameObjects) {
			for (auto& obj : *sceneContext_->activeGameObjects) {
				if (auto* followCam = obj->GetComponent<VirtualFollowCamera>()) {
					followCam->SetPriority(10);
				}
				if (auto* debugCam = obj->GetComponent<VirtualDebugCamera>()) {
					debugCam->SetPriority(20);
				}
			}
		}

		// 一時停止中：エディタを操作できるようにマウスカーソルを出してロック解除
		SetCursorVisible(true);
		SetMouseClip(false);

	} else if (playState_ == EditorPlayState::Pause) {
		playState_ = EditorPlayState::Play; // トグルで再開
		// ゲーム再開：追従カメラを高優先度に戻す
		if (sceneContext_ && sceneContext_->activeGameObjects) {
			for (auto& obj : *sceneContext_->activeGameObjects) {
				if (auto* followCam = obj->GetComponent<VirtualFollowCamera>()) {
					followCam->SetPriority(20);
				}
				if (auto* debugCam = obj->GetComponent<VirtualDebugCamera>()) {
					debugCam->SetPriority(10);
				}
			}
		}

		// 再開：マウスカーソルを消してゲーム画面にロック
		SetCursorVisible(false);
		SetMouseClip(true);
	}
}

void EditorManager::Stop() {
	if (playState_ == EditorPlayState::Edit) return;
	playState_ = EditorPlayState::Edit;

	// 保存してあるスナップショットからシーンを再生前の状態に復元
	if (sceneContext_ && sceneContext_->activeGameObjects && !sceneSnapshot_.empty()) {
		auto& gameObjects = *sceneContext_->activeGameObjects;
		gameObjects.clear();
		ClearSelectedObject();
		if (sceneSnapshot_.contains("objects")) {
			for (const auto& objJ : sceneSnapshot_["objects"]) {
				auto newObj = std::make_unique<GameObject>(sceneContext_, objJ["name"]);
				newObj->Deserialize(objJ);
				newObj->Initialize();
				gameObjects.push_back(std::move(newObj));
			}
		}
		sceneSnapshot_.clear();

		// カメラの参照が外れないように再度紐づけ
		for (auto& obj : gameObjects) {
			if (auto* followCam = obj->GetComponent<VirtualFollowCamera>()) {
				followCam->ResolveTarget(gameObjects);
				followCam->SetPriority(10); // 追従カメラは低優先度
			}
			if (auto* debugCam = obj->GetComponent<VirtualDebugCamera>()) {
				debugCam->SetPriority(20);  // エディタ用デバッグカメラを高優先度
			}
			if (auto* player = obj->GetComponent<PlayerComponent>()) {
				player->ResolveReticle(gameObjects);
			}
		}

		// カメラマネージャーを更新してから、各オブジェクトの描画バッファを初回更新する
		CameraOrganizer::GetInstance()->Update();
		for (auto& obj : gameObjects) {
			obj->UpdateTransformBuffer();
		}
	}

	// 停止（Editモード）：マウスカーソルを表示してロック解除
	SetCursorVisible(true);
	SetMouseClip(false);
}

void EditorManager::DrawMenuBar() {
	if (ImGui::BeginMenuBar()) {
		// Window メニュー
		if (ImGui::BeginMenu("ウィンドウ")) {
			for (auto& window : windows_) {
				ImGui::MenuItem(window->GetName().c_str(), nullptr, window->GetIsOpenPtr());
			}
			ImGui::EndMenu();
		}

		// Layout メニュー
		if (ImGui::BeginMenu("レイアウト")) {
			if (ImGui::MenuItem("レイアウトの保存")) {
				SaveLayoutSettings();
			}
			if (ImGui::MenuItem("レイアウトの読み込み")) {
				LoadLayoutSettings();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}