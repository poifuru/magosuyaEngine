#include "PCH.h"
#include "GameViewWindow.h"
#include "RenderTexture.h"
#include "SrvDescriptorHeapPool.h"
#include "GizmoWindow.h"
#include "CameraOrganizer.h"
#include "MathFunction.h"
#include "CommandManager.h"
#include "TransformCommand.h"
#include "EditorManager.h"

GameViewWindow::GameViewWindow() 
	: IEditorWindow("ゲーム", true) {

}

void GameViewWindow::UpdateAndDraw(const EditorContext& context) {
	// 閉じていたら何もしない
	if(!isOpen_) return;

	// ギズモ操作中はウィンドウが動かないようにする
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
	if(isGizmoActive_) {
		windowFlags |= ImGuiWindowFlags_NoMove;
	}

	// &isOpen_ を渡して右上の×ボタンで閉じられるようにする
	ImGui::Begin(name_.c_str(), &isOpen_, windowFlags);

	// --- 稼働状態変更用のボタン ---
	EditorManager* editorMgr = EditorManager::GetInstance();
	EditorPlayState state = editorMgr->GetPlayState();

	// 横並びに配置
	ImGui::SameLine();
	ImGui::Spacing();
	ImGui::SameLine();

	// --- 再生ボタン ---
	if (state == EditorPlayState::Play) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f)); // 再生中は緑に光らせる！
	}
	if (ImGui::Button(state == EditorPlayState::Play ? "Playing" : " Play > ")) {
		if (state != EditorPlayState::Play) editorMgr->Play();
	}
	if (state == EditorPlayState::Play) {
		ImGui::PopStyleColor();
	}
	ImGui::SameLine();

	// --- 一時停止ボタン ---
	if (state == EditorPlayState::Pause) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.6f, 0.1f, 1.0f)); // 一時停止中は黄色に！
	}
	if (ImGui::Button("Pause ||")) {
		editorMgr->Pause();
	}
	if (state == EditorPlayState::Pause) {
		ImGui::PopStyleColor();
	}
	ImGui::SameLine();

	// --- 停止ボタン ---
	if (ImGui::Button("Stop []")) {
		editorMgr->Stop();
	}
	// --- *** ---

	// アスペクト比選択コンボボックス
	const char* aspectNames[] = { "16:9", "4:3", "Free (Fit)" };
	ImGui::SetNextItemWidth(120.0f);
	ImGui::Combo("アスペクト比", &selectedAspectIndex_, aspectNames, IM_ARRAYSIZE(aspectNames));
	ImGui::Separator();

	// マウス判定
	bool isWinHovered = ImGui::IsWindowHovered();
	isFocused_ = ImGui::IsWindowFocused();

	// ドラッグ判定
	if(isWinHovered && ImGui::IsAnyMouseDown()) {
		isDragging_ = true;
	}
	if(!ImGui::IsAnyMouseDown()) {
		isDragging_ = false;
	}
	isHovered_ = isWinHovered || isDragging_;

	// RenderTextureの描画
	if(context.renderTexture && context.srvHeap) {
		uint32_t srvIndex = context.renderTexture->GetSrvIndex();
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = context.srvHeap->GetGpuHandle(srvIndex);
		ImVec2 availSize = ImGui::GetContentRegionAvail();
		ImVec2 screenPos = ImGui::GetCursorScreenPos();
		float targetAspect = 16.0f / 9.0f;
		bool isAspectFixed = true;
		if(selectedAspectIndex_ == 0) {
			targetAspect = 16.0f / 9.0f;
		}
		else if(selectedAspectIndex_ == 1) {
			targetAspect = 4.0f / 3.0f;
		}
		else {
			isAspectFixed = false;
		}
		ImVec2 imageSize = availSize;

		// アスペクト比固定のセンタリング計算
		if(isAspectFixed && availSize.y > 0.0f) {
			float availAspect = availSize.x / availSize.y;
			if(availAspect > targetAspect) {
				imageSize.y = availSize.y;
				imageSize.x = availSize.y * targetAspect;
			}
			else {
				imageSize.x = availSize.x;
				imageSize.y = availSize.x / targetAspect;
			}
			float offsetX = (availSize.x - imageSize.x) * 0.5f;
			float offsetY = (availSize.y - imageSize.y) * 0.5f;
			ImVec2 cursorPos = ImGui::GetCursorPos();
			cursorPos.x += offsetX;
			cursorPos.y += offsetY;
			ImGui::SetCursorPos(cursorPos);
			screenPos.x += offsetX;
			screenPos.y += offsetY;
		}
		gameScreenPos_ = screenPos;
		gameScreenSize_ = imageSize;

		// ゲームプレイ中なら、ゲーム画面の矩形にマウスを常時ロック追従
		if (EditorManager::GetInstance()->IsPlaying() && imageSize.x > 0.0f && imageSize.y > 0.0f) {
			RECT rect;
			rect.left = static_cast<LONG>(screenPos.x);
			rect.top = static_cast<LONG>(screenPos.y);
			rect.right = static_cast<LONG>(screenPos.x + imageSize.x);
			rect.bottom = static_cast<LONG>(screenPos.y + imageSize.y);
			ClipCursor(&rect);
		}

		// 画像描画
		ImGui::Image((ImTextureID)gpuHandle.ptr, imageSize);

		// ギズモ描画
		GameObject* selectedObject = EditorManager::GetInstance()->GetSelectedObject();
		if(selectedObject != nullptr) {
			auto& camData = CameraOrganizer::GetInstance()->GetCameraData();
			auto& transform = selectedObject->GetTransform();
			Matrix4x4 worldMatrix = Math::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::BeginFrame();
			ImGuizmo::AllowAxisFlip(false);

			// GizmoWindow から現在の操作モードを取得
			ImGuizmo::OPERATION currentOp = ImGuizmo::TRANSLATE;
			ImGuizmo::MODE currentMode = ImGuizmo::LOCAL;
			if(auto* gizmoWin = EditorManager::GetInstance()->GetWindow<GizmoWindow>()) {
				currentOp = gizmoWin->GetOperation();
				currentMode = gizmoWin->GetMode();
			}

			// 描画範囲をゲーム画面に合わせる
			ImGuizmo::SetRect(gameScreenPos_.x, gameScreenPos_.y, gameScreenSize_.x, gameScreenSize_.y);

			// 右手系補正
			Matrix4x4 projGizmo = camData.proj;
			projGizmo.m[2][2] = projGizmo.m[2][2] * 2.0f - projGizmo.m[2][3];
			projGizmo.m[3][2] = projGizmo.m[3][2] * 2.0f;
			ImGuizmo::SetAlternativeWindow(ImGui::GetCurrentWindow());
			ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
			isGizmoActive_ = ImGuizmo::IsOver() || ImGuizmo::IsUsing();
			static EulerTransform transformBeforeDrag;
			static bool wasUsingGizmo = false;
			if(ImGuizmo::IsOver() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				transformBeforeDrag = transform;
			}
			ImGuizmo::Manipulate(
				&camData.view.m[0][0],
				&projGizmo.m[0][0],
				currentOp,
				currentMode,
				&worldMatrix.m[0][0]
			);
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
		ImGui::End();
	}
}
