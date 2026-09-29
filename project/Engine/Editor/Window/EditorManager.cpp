#include "PCH.h"
#include "EditorManager.h"
#include "RenderTexture.h"
#include "SrvDescriptorHeap.h"

// 各ウィンドウクラス
#include "PerformanceWindow.h"

void EditorManager::Initialize() {
	// ウィンドウを登録
	RegisterWindow<PerformanceWindow>();

	// 登録したウィンドウを初期化
	for(auto& window : windows_) {
		window->Initialize();
	}

	// 前回の開閉状態を復元
	LoadLayoutSettings();
}

void EditorManager::Finalize() {
	// 終了時に状態を保存
	SaveLayoutSettings();
}

void EditorManager::UpdateAndDraw(
	ID3D12Device* device,
	MyEngine::LowLevel::SrvDescriptorHeap* heapManager,
	MyEngine::Rendering::RenderTexture* renderTexture
) {
#ifdef USEIMGUI
	//*** 各ウィンドウを順番に描画していく ***//
	
	// メニューバー
	DrawMenuBar();
	
	// ゲームウィンドウ
	DrawGameWindow(heapManager, renderTexture);

	// EditorContextを詰めて、登録された各ウィンドウを描画
	EditorContext context{};
	context.device = device;
	context.heapManager = heapManager;
	context.renderTexture = renderTexture;
	for (auto& window : windows_) {
		window->UpdateAndDraw(context);
	}
#endif
}

void EditorManager::SaveLayoutSettings() {
	nlohmann::json root;
	root["windows"] = nlohmann::json::object();
	for (auto& window : windows_) {
		root["windows"][window->GetName()] = window->IsOpen();
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
	if (root.contains("windows") && root["windows"].is_object()) {
		for (auto& window : windows_) {
			const std::string& name = window->GetName();
			if (root["windows"].contains(name)) {
				window->SetOpen(root["windows"][name].get<bool>());
			}
		}
	}
}

void EditorManager::DrawMenuBar() {
	if (ImGui::BeginMenuBar()) {
		// Window メニュー
		if (ImGui::BeginMenu("Window")) {
			for (auto& window : windows_) {
				ImGui::MenuItem(window->GetName().c_str(), nullptr, window->GetIsOpenPtr());
			}
			ImGui::EndMenu();
		}

		// Layout メニュー
		if (ImGui::BeginMenu("Layout")) {
			if (ImGui::MenuItem("Save Layout")) {
				SaveLayoutSettings();
			}
			if (ImGui::MenuItem("Load Layout")) {
				LoadLayoutSettings();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
}

void EditorManager::DrawGameWindow(
	MyEngine::LowLevel::SrvDescriptorHeap* heapManager,
	MyEngine::Rendering::RenderTexture* renderTexture
) {
	// ギズモ操作中はウィンドウが動かないようにする
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
	if (isGizmoActive_) {
		windowFlags |= ImGuiWindowFlags_NoMove;
	}

	// ゲーム画面をImGuiウィンドウとして描画する
	ImGui::Begin("Game", nullptr, windowFlags);

	// アスペクト比選択コンボボックスの配置
	const char* aspectNames[] = { "16:9", "4:3", "Free (Fit)" };
	ImGui::SetNextItemWidth(120.0f);
	ImGui::Combo("Aspect", &selectedAspectIndex_, aspectNames, IM_ARRAYSIZE(aspectNames));
	ImGui::Separator();

	// 純粋にウィンドウ上にマウスがあるか
	bool isHovered = ImGui::IsWindowHovered();
	isGameWindowFocused_ = ImGui::IsWindowFocused();

	// ドラッグ開始判定：ウィンドウ上でクリックされたらドラッグ中フラグをON
	if (isHovered && ImGui::IsAnyMouseDown()) {
		isGameWindowDragging_ = true;
	}

	// ドラッグ終了判定：マウスボタンが全て離されたらフラグをOFF
	if (!ImGui::IsAnyMouseDown()) {
		isGameWindowDragging_ = false;
	}

	// 「ウィンドウ上にマウスがある」か「ゲームウィンドウからドラッグ中」なら、ホバー状態とみなす
	isGameWindowHovered_ = isHovered || isGameWindowDragging_;

	// RenderTextureのSRVからGPUハンドルを取得
	if(renderTexture) {
		uint32_t srvIndex = renderTexture->GetSrvIndex();
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = heapManager->GetGpuHandle(srvIndex);

		// ウィンドウで現在利用可能な領域を取得
		ImVec2 availSize = ImGui::GetContentRegionAvail();

		// 現在の描画カーソルのスクリーン座標（絶対座標）を取得
		ImVec2 screenPos = ImGui::GetCursorScreenPos();

		// 選択されたアスペクト比のターゲットを決定
		float targetAspect = 16.0f / 9.0f;
		bool isAspectFixed = true;
		if(selectedAspectIndex_ == 0) {
			targetAspect = 16.0f / 9.0f;
		}
		else if(selectedAspectIndex_ == 1) {
			targetAspect = 4.0f / 3.0f;
		}
		else {
			isAspectFixed = false; // 自由変形
		}
		ImVec2 imageSize = availSize;

		// アスペクト比を固定する場合のサイズ計算
		if(isAspectFixed && availSize.y > 0.0f) {
			float availAspect = availSize.x / availSize.y;
			if(availAspect > targetAspect) {
				// ウィンドウが横長すぎる場合 ➔ 高さに合わせる
				imageSize.y = availSize.y;
				imageSize.x = availSize.y * targetAspect;
			}
			else {
				// ウィンドウが縦長すぎる場合 ➔ 幅に合わせる
				imageSize.x = availSize.x;
				imageSize.y = availSize.x / targetAspect;
			}

			// 画面をウィンドウ中央に寄せる(センタリング)
			float offsetX = (availSize.x - imageSize.x) * 0.5f;
			float offsetY = (availSize.y - imageSize.y) * 0.5f;

			ImVec2 cursorPos = ImGui::GetCursorPos();
			cursorPos.x += offsetX;
			cursorPos.y += offsetY;
			ImGui::SetCursorPos(cursorPos);

			// スクリーン座標も中央寄せ分ずらす
			screenPos.x += offsetX;
			screenPos.y += offsetY;
		}

		// 実際の描画位置とサイズをメンバ変数に保存
		gameScreenPos_ = screenPos;
		gameScreenSize_ = imageSize;

		// 計算したサイズで描画
		ImGui::Image((ImTextureID)gpuHandle.ptr, imageSize);
	}

	ImGui::End();
}