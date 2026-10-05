#include "PCH.h"
#include "HierarchyWindow.h"
#include "GameObject.h"
#include "EditorManager.h"
#include "BaseScene.h"
#include "VirtualFollowCamera.h"
#include "PlayerComponent.h"
#include "TitleScene.h"
#include "PlayScene.h"
#include "SceneManager.h"

HierarchyWindow::HierarchyWindow() 
	: IEditorWindow("ヒエラルキー", true) {
}

void HierarchyWindow::UpdateAndDraw(const EditorContext& context) {
	if (!isOpen_) return;

	SceneContext* sceneCtx = context.sceneContext;
	if (!sceneCtx || !sceneCtx->activeGameObjects) {
		ImGui::Begin(name_.c_str(), &isOpen_);
		ImGui::Text("シーンがロードされていません");
		ImGui::End();
		return;
	}

	auto& gameObjects = *sceneCtx->activeGameObjects;
	GameObject* currentSelected = EditorManager::GetInstance()->GetSelectedObject();

	ImGui::Begin(name_.c_str(), &isOpen_);
	// --- シーン保存 / 読込 / 新規 ボタン ---
	if (ImGui::Button("シーンを保存")) {
		ImGui::OpenPopup("シーンを保存");
	}
	ImGui::SameLine();

	if (ImGui::Button("シーン読み込み")) {
		ImGui::OpenPopup("シーン読み込み");
	}

	if (ImGui::Button("新規シーン")) {
		ImGui::OpenPopup("新規シーン作成確認");
	}
	ImGui::Separator();

	// --- 保存モーダル ---
	if (ImGui::BeginPopupModal("シーンを保存", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("保存するシーンファイル名を入力してください。");
		ImGui::Spacing();
		ImGui::InputText("ファイル名", saveFileName_, sizeof(saveFileName_));
		ImGui::Spacing();

		if (ImGui::Button("保存", ImVec2(120, 0))) {
			std::string fileToSave = saveFileName_;
			if (fileToSave.find(".json") == std::string::npos) {
				fileToSave += ".json";
			}

			const std::string sceneFolder = "Resources/Scene";
			if (!std::filesystem::exists(sceneFolder)) {
				std::filesystem::create_directories(sceneFolder);
			}

			SaveScene(sceneFolder + "/" + fileToSave, gameObjects);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();

		if (ImGui::Button("キャンセル", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	// --- 読み込みモーダル ---
	if (ImGui::BeginPopupModal("シーン読み込み", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("読み込むシーンファイルを選択してください。");
		ImGui::Spacing();

		const std::string sceneFolder = "Resources/Scene";
		if (!std::filesystem::exists(sceneFolder)) {
			std::filesystem::create_directories(sceneFolder);
		}

		std::vector<std::string> sceneFiles;
		for (const auto& entry : std::filesystem::directory_iterator(sceneFolder)) {
			if (entry.is_regular_file() && entry.path().extension() == ".json") {
				sceneFiles.push_back(entry.path().filename().string());
			}
		}

		std::vector<const char*> sceneFileNames;
		for (const auto& name : sceneFiles) {
			sceneFileNames.push_back(name.c_str());
		}

		if (!sceneFileNames.empty()) {
			if (selectedSceneFileIndex_ >= static_cast<int>(sceneFileNames.size())) {
				selectedSceneFileIndex_ = 0;
			}
			ImGui::Combo("ファイル名", &selectedSceneFileIndex_, sceneFileNames.data(), static_cast<int>(sceneFileNames.size()));
			ImGui::Spacing();

			if (ImGui::Button("読み込み", ImVec2(120, 0))) {
				std::string fileToLoad = sceneFolder + "/" + sceneFiles[selectedSceneFileIndex_];
				LoadScene(fileToLoad, gameObjects, sceneCtx);
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
		}
		else {
			ImGui::Text("シーンファイル (*.json) が見つかりません。");
		}
		if (ImGui::Button("キャンセル", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	// --- 新規シーン確認モーダル ---
	if (ImGui::BeginPopupModal("新規シーン作成確認", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("現在のシーンのGameObjectはすべて破棄されます。\nよろしいですか？");
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (ImGui::Button("作成", ImVec2(120, 0))) {
			gameObjects.clear();
			EditorManager::GetInstance()->ClearSelectedObject();
			strcpy_s(saveFileName_, "defaultScene.json");
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();

		if (ImGui::Button("キャンセル", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	// --- シーン切り替えデバッグ ---
	ImGui::Separator();
	ImGui::Text("シーン切り替え (Debug)");
	static float transitionDuration = 1.0f;
	ImGui::DragFloat("遷移秒数", &transitionDuration, 0.1f, 0.0f, 5.0f, "%.1f 秒");
	if (sceneCtx->sceneManager) {
		if (ImGui::Button("TitleScene へ")) {
			// ディゾルブ遷移を呼び出す！（スライダーの秒数を渡す）
			sceneCtx->sceneManager->ChangeSceneWithDissolve<TitleScene>(transitionDuration, transitionDuration);
			ImGui::End();
			return; // ★ シーンが変わったらこのフレームのヒエラルキー描画は即座に終了
		}
		ImGui::SameLine();
		if (ImGui::Button("PlayScene へ")) {
			// ディゾルブ遷移を呼び出す！
			sceneCtx->sceneManager->ChangeSceneWithDissolve<PlayScene>(transitionDuration, transitionDuration);
			ImGui::End();
			return; // ★ 安全のため即座に終了！
		}
		// 遷移中ならプログレスバーを出す
		if (sceneCtx->sceneManager->isTransitioning()) {
			float progress = sceneCtx->sceneManager->GetTransitionPogress();
			ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f));
		}
	}
	ImGui::Separator();

	// --- 新規 GameObject 作成ボタン ---
	if (ImGui::Button("新規 GameObject 作成")) {
		gameObjects.push_back(std::make_unique<GameObject>(sceneCtx, "New GameObject"));
	}
	ImGui::Spacing();

	// --- オブジェクト一覧リスト ---
	for (auto it = gameObjects.begin(); it != gameObjects.end(); ) {
		GameObject* obj = it->get();
		bool isSelected = (currentSelected == obj);
		ImGui::PushID(obj);

		// 削除ボタン
		if (ImGui::Button("X")) {
			if (currentSelected == obj) {
				EditorManager::GetInstance()->ClearSelectedObject();
			}
			it = gameObjects.erase(it);
			ImGui::PopID();
			continue;
		}
		ImGui::SameLine();

		// 選択項目
		if (ImGui::Selectable(obj->GetName().c_str(), isSelected)) {
			EditorManager::GetInstance()->SetSelectedObject(obj); // ★ 選択を更新！
		}

		ImGui::PopID();
		++it;
	}
	ImGui::End();
}

void HierarchyWindow::SaveScene(
	const std::string& fileName, 
	const std::vector<std::unique_ptr<GameObject>>& gameObjects
) {
	json sceneJ;
	sceneJ["name"] = "scene";
	sceneJ["objects"] = json::array();
	for (const auto& obj : gameObjects) {
		if (obj->IsSerializable()) {
			sceneJ["objects"].push_back(obj->Serialize());
		}
	}
	std::ofstream file(fileName);
	if (file.is_open()) {
		file << sceneJ.dump(4);
	}
	std::string nameOnly = std::filesystem::path(fileName).filename().string();
	strcpy_s(saveFileName_, nameOnly.c_str());
}

void HierarchyWindow::LoadScene(
	const std::string& fileName,
	std::vector<std::unique_ptr<GameObject>>& gameObjects,
	SceneContext* sceneContext
) {
	gameObjects.clear();
	EditorManager::GetInstance()->ClearSelectedObject();
	std::ifstream file(fileName);
	if (!file.is_open()) return;
	json sceneJ;
	file >> sceneJ;
	std::string nameOnly = std::filesystem::path(fileName).filename().string();
	strcpy_s(saveFileName_, nameOnly.c_str());
	if (sceneJ.contains("objects")) {
		for (const auto& objJ : sceneJ["objects"]) {
			auto newObj = std::make_unique<GameObject>(sceneContext, objJ["name"]);
			newObj->Deserialize(objJ);
			newObj->Initialize();
			gameObjects.push_back(std::move(newObj));
		}
		for (auto& obj : gameObjects) {
			if (auto* followCam = obj->GetComponent<VirtualFollowCamera>()) {
				followCam->ResolveTarget(gameObjects);
			}
			if (auto* player = obj->GetComponent<PlayerComponent>()) {
				player->ResolveReticle(gameObjects);
			}
		}
	}
}
