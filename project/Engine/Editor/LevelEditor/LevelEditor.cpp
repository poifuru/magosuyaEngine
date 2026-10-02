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