#include "PCH.h"
#include "EditorManager.h"
#include "RenderTexture.h"
#include "SrvDescriptorHeapPool.h"
#include "CommandManager.h"

// 各ウィンドウクラス
#include "PerformanceWindow.h"
#include "GameViewWindow.h"
#include "AssetBrowserWindow.h"
#include "HierarchyWindow.h"
#include "InspectorWindow.h"
#include "GizmoWindow.h"

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