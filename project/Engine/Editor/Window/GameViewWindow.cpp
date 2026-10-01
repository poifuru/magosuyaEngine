#include "PCH.h"
#include "GameViewWindow.h"
#include "RenderTexture.h"
#include "SrvDescriptorHeapPool.h"

GameViewWindow::GameViewWindow() 
	: IEditorWindow("ゲーム", true) {

}

void GameViewWindow::UpdateAndDraw(const EditorContext& context) {
	// 閉じていたら何もしない
	if (!isOpen_) return;

	// ギズモ操作中はウィンドウが動かないようにする
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
	if (isGizmoActive_) {
		windowFlags |= ImGuiWindowFlags_NoMove;
	}

	// &isOpen_ を渡して右上の×ボタンで閉じられるようにする
	ImGui::Begin(name_.c_str(), &isOpen_, windowFlags);

	// アスペクト比選択コンボボックス
	const char* aspectNames[] = { "16:9", "4:3", "Free (Fit)" };
	ImGui::SetNextItemWidth(120.0f);
	ImGui::Combo("アスペクト比", &selectedAspectIndex_, aspectNames, IM_ARRAYSIZE(aspectNames));
	ImGui::Separator();

	// マウス判定
	bool isWinHovered = ImGui::IsWindowHovered();
	isFocused_ = ImGui::IsWindowFocused();

	// ドラッグ判定
	if (isWinHovered && ImGui::IsAnyMouseDown()) {
		isDragging_ = true;
	}
	if (!ImGui::IsAnyMouseDown()) {
		isDragging_ = false;
	}
	isHovered_ = isWinHovered || isDragging_;

	// RenderTextureの描画
	if (context.renderTexture && context.srvHeap) {
		uint32_t srvIndex = context.renderTexture->GetSrvIndex();
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = context.srvHeap->GetGpuHandle(srvIndex);
		ImVec2 availSize = ImGui::GetContentRegionAvail();
		ImVec2 screenPos = ImGui::GetCursorScreenPos();
		float targetAspect = 16.0f / 9.0f;
		bool isAspectFixed = true;
		if (selectedAspectIndex_ == 0) {
			targetAspect = 16.0f / 9.0f;
		}
		else if (selectedAspectIndex_ == 1) {
			targetAspect = 4.0f / 3.0f;
		}
		else {
			isAspectFixed = false;
		}
		ImVec2 imageSize = availSize;

		// アスペクト比固定のセンタリング計算
		if (isAspectFixed && availSize.y > 0.0f) {
			float availAspect = availSize.x / availSize.y;
			if (availAspect > targetAspect) {
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

		// 画像描画
		ImGui::Image((ImTextureID)gpuHandle.ptr, imageSize);
	}
	ImGui::End();
}
