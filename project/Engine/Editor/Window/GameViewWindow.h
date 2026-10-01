#pragma once
#include "IEditorWindow.h"

class GameViewWindow : public IEditorWindow {
public:
	// コンストラクタ
	GameViewWindow();

	// デストラクタ
	~GameViewWindow() override = default;

	// 更新と描画
	void UpdateAndDraw(const EditorContext& context) override;

	// 外部（ギズモなど）から参照されるゲッター / セッター
	bool IsHovered() const { return isHovered_; }
	bool IsFocused() const { return isFocused_; }

	ImVec2 GetGameScreenPos() const { return gameScreenPos_; }
	ImVec2 GetGameScreenSize() const { return gameScreenSize_; }

	void SetGizmoActive(bool active) { isGizmoActive_ = active; }
	bool IsGizmoActive() const { return isGizmoActive_; }

private:
	bool isHovered_ = false;
	bool isFocused_ = false;
	bool isDragging_ = false;
	bool isGizmoActive_ = false;

	// アスペクト比変更用 (0: 16:9, 1: 4:3, 2: 自由)
	int selectedAspectIndex_ = 0;

	ImVec2 gameScreenPos_ = { 0.0f, 0.0f };
	ImVec2 gameScreenSize_ = { 0.0f, 0.0f };
};