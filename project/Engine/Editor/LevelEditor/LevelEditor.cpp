#include "PCH.h"
#include "LevelEditor.h"
#include "CommandManager.h"
#include "TransformCommand.h"
#include "MeshRendererComponent.h"
#include "MathFunction.h"
#include "BaseCamera.h"
#include "EditorManager.h"
#include "VirtualFollowCamera.h"
#include "PlayerComponent.h"
#include "SkyboxComponent.h"
#include "WaterSurfaceComponent.h"
#include "SpriteComponent.h"

void LevelEditor::Initialize(SceneContext* context) {
	context_ = context;
}

void LevelEditor::Update(std::vector<std::unique_ptr<GameObject>>& gameObjects, GameObject*& selectedObject, CameraData* cameraData) {
	if(!context_) return;

	// Ctrl + Z で Undo (元に戻す)
	if(ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) {
		CommandManager::GetInstance()->Undo();
	}
	// Ctrl + Y で Redo (やり直す)
	if(ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) {
		CommandManager::GetInstance()->Redo();
	}
	// ---- ヒエラルキー ----
	ImGui::Begin("ヒエラルキー");

	if(ImGui::Button("シーンを保存")) {
		ImGui::OpenPopup("シーンを保存"); // ポップアップを開くトリガー
	}

	ImGui::SameLine();

	if(ImGui::Button("シーン読み込み")) {
		ImGui::OpenPopup("シーン読み込み");
	}

	if(ImGui::Button("新規シーン")) {
		ImGui::OpenPopup("新規シーン作成確認");
	}

	ImGui::Separator();

	// 保存用のモーダルポップアップ画面
	if(ImGui::BeginPopupModal("シーンを保存", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("保存するシーンファイル名を入力してください。");
		ImGui::Spacing();

		ImGui::InputText("ファイル名", saveFileName_, sizeof(saveFileName_));
		ImGui::Spacing();

		if(ImGui::Button("保存", ImVec2(120, 0))) {
			std::string fileToSave = saveFileName_;
			if(fileToSave.find(".json") == std::string::npos) {
				fileToSave += ".json";
			}

			const std::string sceneFolder = "Resources/Scene";
			if(!std::filesystem::exists(sceneFolder)) {
				std::filesystem::create_directories(sceneFolder);
			}

			std::string fullPath = sceneFolder + "/" + fileToSave;
			SaveScene(fullPath, gameObjects);
			ImGui::CloseCurrentPopup(); // ポップアップを閉じる
		}

		ImGui::SameLine();

		if(ImGui::Button("キャンセル", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup(); // ポップアップを閉じる
		}

		ImGui::EndPopup();
	}

	// 読み込み用のモーダルポップアップ画面
	if(ImGui::BeginPopupModal("シーン読み込み", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("読み込むシーンファイルを選択してください。");
		ImGui::Spacing();

		// Resources/Scene内の .json ファイルをスキャンする
		const std::string sceneFolder = "Resources/Scene";
		if(!std::filesystem::exists(sceneFolder)) {
			std::filesystem::create_directories(sceneFolder);
		}

		std::vector<std::string> sceneFiles;
		for(const auto& entry : std::filesystem::directory_iterator(sceneFolder)) {
			if(entry.is_regular_file() && entry.path().extension() == ".json") {
				sceneFiles.push_back(entry.path().filename().string());
			}
		}

		// ImGuiのCombo用に const char* の配列を作る
		std::vector<const char*> sceneFileNames;
		for(const auto& name : sceneFiles) {
			sceneFileNames.push_back(name.c_str());
		}

		if(!sceneFileNames.empty()) {
			if(selectedSceneFileIndex_ >= static_cast<int>(sceneFileNames.size())) {
				selectedSceneFileIndex_ = 0;
			}

			ImGui::Combo("ファイル名", &selectedSceneFileIndex_, sceneFileNames.data(), static_cast<int>(sceneFileNames.size()));

			ImGui::Spacing();

			if(ImGui::Button("読み込み", ImVec2(120, 0))) {
				std::string fileToLoad = sceneFolder + "/" + sceneFiles[selectedSceneFileIndex_];
				LoadScene(fileToLoad, gameObjects, selectedObject);
				ImGui::CloseCurrentPopup(); // ポップアップを閉じる
			}

			ImGui::SameLine();

		}
		else {
			ImGui::Text("シーンファイル (*.json) が見つかりません。");
		}

		if(ImGui::Button("キャンセル", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup(); // ポップアップを閉じる
		}

		ImGui::EndPopup();
	}

	// 新規シーン作成の確認用モーダル
	if(ImGui::BeginPopupModal("新規シーン作成確認", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("現在のシーンのGameObjectはすべて破棄されます。\nよろしいですか？");
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		if(ImGui::Button("作成", ImVec2(120, 0))) {
			gameObjects.clear();          // 全オブジェクト削除
			selectedObject = nullptr;     // 選択中のポインタをクリア
			strcpy_s(saveFileName_, "defaultScene.json"); // 保存ファイル名もデフォルトに戻す
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if(ImGui::Button("キャンセル", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	// 新規GameObject作成ボタン
	if(ImGui::Button("新規 GameObject 作成")) {
		gameObjects.push_back(std::make_unique<GameObject>(context_, "New GameObject"));
	}
	ImGui::Spacing();
	// オブジェクト一覧を表示
	for(auto it = gameObjects.begin(); it != gameObjects.end(); ) {
		GameObject* obj = it->get();
		bool isSelected = (selectedObject == obj);
		ImGui::PushID(obj);
		// 削除ボタン
		if(ImGui::Button("X")) {
			if(selectedObject == obj) {
				selectedObject = nullptr;
			}
			it = gameObjects.erase(it);
			ImGui::PopID();
			continue;
		}
		ImGui::SameLine();
		// 選択用のSelectable項目
		if(ImGui::Selectable(obj->GetName().c_str(), isSelected)) {
			selectedObject = obj;
		}
		ImGui::PopID();
		++it;
	}
	ImGui::End();
	// ---- インスペクター ----
	ImGui::Begin("Inspector");
	if(selectedObject != nullptr) {
		selectedObject->ImGui();
	}
	else {
		ImGui::Text("オブジェクトが選択されていません");
	}
	ImGui::End();
	// ---- 3Dギズモの描画 ----
	if(selectedObject != nullptr) {
		auto& camData = *cameraData;
		auto& transform = selectedObject->GetTransform();
		// ワールド行列を作成
		Matrix4x4 worldMatrix = Math::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::BeginFrame();
		ImGuizmo::AllowAxisFlip(false);
		static ImGuizmo::OPERATION currentGizmoOperation(ImGuizmo::TRANSLATE);
		static ImGuizmo::MODE currentGizmoMode(ImGuizmo::LOCAL);
		ImGuiIO& io = ImGui::GetIO();

		// "Game" ウィンドウをアペンドオープンする
		ImGui::Begin("Game");

		// ギズモ操作用の設定用ウィンドウ
		ImGui::Begin("Gizmo");
		ImGui::Text("Gizmo Operation");
		if(ImGui::RadioButton("S", currentGizmoOperation == ImGuizmo::SCALE)) {
			currentGizmoOperation = ImGuizmo::SCALE;
		}
		ImGui::SameLine();
		if(ImGui::RadioButton("R", currentGizmoOperation == ImGuizmo::ROTATE)) {
			currentGizmoOperation = ImGuizmo::ROTATE;
		}
		ImGui::SameLine();
		if(ImGui::RadioButton("T", currentGizmoOperation == ImGuizmo::TRANSLATE)) {
			currentGizmoOperation = ImGuizmo::TRANSLATE;
		}

		ImGui::Text("Gizmo Space");
		if(ImGui::RadioButton("Local", currentGizmoMode == ImGuizmo::LOCAL)) {
			currentGizmoMode = ImGuizmo::LOCAL;
		}
		ImGui::SameLine();
		if(ImGui::RadioButton("World", currentGizmoMode == ImGuizmo::WORLD)) {
			currentGizmoMode = ImGuizmo::WORLD;
		}
		ImGui::End();

		// 2. ギズモの描画範囲を EditorManager で計算された実際の画面に合わせる
		ImVec2 screenPos = EditorManager::GetInstance()->GetGameScreenPos();
		ImVec2 screenSize = EditorManager::GetInstance()->GetGameScreenSize();
		ImGuizmo::SetRect(screenPos.x, screenPos.y, screenSize.x, screenSize.y);

		// 座標変換 of 右手系補正
		Matrix4x4 projGizmo = camData.proj;
		projGizmo.m[2][2] = projGizmo.m[2][2] * 2.0f - projGizmo.m[2][3];
		projGizmo.m[3][2] = projGizmo.m[3][2] * 2.0f;

		// 入力判定用の代替ウィンドウとして "Game" ウィンドウをセットする
		ImGuizmo::SetAlternativeWindow(ImGui::GetCurrentWindow());

		// ギズモの上にマウスがあるか、ドラッグ操作中のときにフラグを立てて、
		// 次のフレームのGameウィンドウドラッグ移動を防止する
		bool isGizmoActive = ImGuizmo::IsOver() || ImGuizmo::IsUsing();
		EditorManager::GetInstance()->SetGizmoActive(isGizmoActive);

		static EulerTransform transformBeforeDrag;
		static bool wasUsingGizmo = false;

		if(ImGuizmo::IsOver() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			transformBeforeDrag = transform;
		}

		// 3. 描画は最前面に
		ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

		ImGuizmo::Manipulate(
			&camData.view.m[0][0],
			&projGizmo.m[0][0],
			currentGizmoOperation,
			currentGizmoMode,
			&worldMatrix.m[0][0]
		);

		ImGui::End();

		if(ImGuizmo::IsUsing()) {
			wasUsingGizmo = true;
			float matrixTranslation[3], matrixRotation[3], matrixScale[3];
			ImGuizmo::DecomposeMatrixToComponents(&worldMatrix.m[0][0], matrixTranslation, matrixRotation, matrixScale);
			transform.translate = { matrixTranslation[0], matrixTranslation[1], matrixTranslation[2] };
			const float DEG_TO_RAD = 3.14159265f / 180.0f;
			transform.rotate = {
				matrixRotation[0] * DEG_TO_RAD,
				matrixRotation[1] * DEG_TO_RAD,
				matrixRotation[2] * DEG_TO_RAD
			};
			transform.scale = { matrixScale[0], matrixScale[1], matrixScale[2] };
		}
		else if(wasUsingGizmo) {
			wasUsingGizmo = false;
			auto command = std::make_unique<TransformCommand>(selectedObject, transformBeforeDrag, transform);
			CommandManager::GetInstance()->AddAndExecute(std::move(command));
		}
	}
}

void LevelEditor::SaveScene(const std::string& fileName,
							const std::vector<std::unique_ptr<GameObject>>& gameObjects
) {
	json sceneJ;
	sceneJ["name"] = "scene";
	sceneJ["objects"] = json::array();
	for(const auto& obj : gameObjects) {
		if(obj->IsSerializable()) { // 保存対象だけを保存
			sceneJ["objects"].push_back(obj->Serialize());
		}
	}
	std::ofstream file(fileName);
	if(file.is_open()) {
		file << sceneJ.dump(4);
		file.close();
	}

	// セーブしたファイル名をバッファに記憶する
	std::string nameOnly = std::filesystem::path(fileName).filename().string();
	strcpy_s(saveFileName_, nameOnly.c_str());
}

void LevelEditor::LoadScene(const std::string& fileName,
							std::vector<std::unique_ptr<GameObject>>& gameObjects,
							GameObject*& selectedObject
) {
	gameObjects.clear();
	selectedObject = nullptr;
	std::ifstream file(fileName);
	if(file.is_open()) {
		json sceneJ;
		file >> sceneJ;
		file.close();

		// ロードしたファイル名を保存デフォルト名にする
		std::string nameOnly = std::filesystem::path(fileName).filename().string();
		strcpy_s(saveFileName_, nameOnly.c_str());

		if(sceneJ.contains("objects")) {
			for(const auto& objJ : sceneJ["objects"]) {
				auto newObj = std::make_unique<GameObject>(context_, objJ["name"]);
				newObj->Deserialize(objJ);
				newObj->Initialize();
				gameObjects.push_back(std::move(newObj));
			}
			// 全てのオブジェクトが読み込まれた後に紐づけを実行する
			for(auto& obj : gameObjects) {
				if(auto* followCam = obj->GetComponent<VirtualFollowCamera>()) {
					followCam->ResolveTarget(gameObjects);
				}
				if(auto* player = obj->GetComponent<PlayerComponent>()) {
					player->ResolveReticle(gameObjects);
				}
			}
		}
	}
}