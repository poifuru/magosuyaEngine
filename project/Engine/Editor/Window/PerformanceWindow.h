#pragma once
#include "IEditorWindow.h"

class PerformanceWindow : public IEditorWindow {
public:
	// コンストラクタ
	PerformanceWindow();

	// デストラクタ
	~PerformanceWindow() = default;

	// 更新と描画
	void UpdateAndDraw(const EditorContext& context) override;
};