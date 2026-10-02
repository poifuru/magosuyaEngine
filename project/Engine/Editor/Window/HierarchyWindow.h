#pragma once
#include "IEditorWindow.h"

class GameObject;

class HierarchyWindow : public IEditorWindow {
public:
	// コンストラクタ
	HierarchyWindow();

	// デストラクタ
	~HierarchyWindow() override = default;

	// 更新と描画
	void UpdateAndDraw(const EditorContext& context) override;

private:
	// シーンを保存
	void SaveScene(const std::string& fileName, const std::vector<std::unique_ptr<GameObject>>& gameObjects);

	// シーンを読み込み
	void LoadScene(const std::string& fileName, std::vector<std::unique_ptr<GameObject>>& gameObjects, SceneContext* sceneContext);

private:
	char saveFileName_[128] = "defaultScene.json";
	int selectedSceneFileIndex_ = 0;
};