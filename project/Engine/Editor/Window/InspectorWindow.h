#pragma once
#include "IEditorWindow.h"

class InspectorWindow : public IEditorWindow {
public:
	// コンストラクタ
	InspectorWindow();

	// デストラクタ
	~InspectorWindow() override = default;

	// 更新と描画
	void UpdateAndDraw(const EditorContext& context) override;
};