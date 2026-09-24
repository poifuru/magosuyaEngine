#pragma once
#include "IEditorWindow.h"

class PerformanceWindow : public IEditorWindow {
public:
	PerformanceWindow();
	~PerformanceWindow() = default;
	
	void Initialize() override;

	void UpdateAndDraw(const EditorContext& context) override;
};