#pragma once
#include "IEditorWindow.h"
#include <imGuizmo.h>

class GizmoWindow : public IEditorWindow {
public:
	// コンストラクタ
	GizmoWindow();

	// デストラクタ
	~GizmoWindow() override = default;

	// 更新と描画
	void UpdateAndDraw(const EditorContext& context) override;

	ImGuizmo::OPERATION GetOperation() const { return currentOperation_; }
	ImGuizmo::MODE GetMode() const { return currentMode_; }

private:
	ImGuizmo::OPERATION currentOperation_ = ImGuizmo::TRANSLATE;
	ImGuizmo::MODE currentMode_ = ImGuizmo::LOCAL;
};

