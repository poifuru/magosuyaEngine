#pragma once
#include "GameObject.h"
#include "struct.h"

struct SceneContext;
struct CameraData;

class LevelEditor {
public:
	LevelEditor() = default;
	~LevelEditor() = default;
	void Initialize(SceneContext* context);

	// PlaySceneのGameObjectリストと、選択中のオブジェクトのポインタを渡して更新する
	void Update(
		std::vector<std::unique_ptr<GameObject>>& gameObjects, 
		GameObject*& selectedObject, 
		const CameraData* cameraData
	);

	void SaveScene(const std::string& fileName, 
				   const std::vector<std::unique_ptr<GameObject>>& gameObjects
	);

	void LoadScene(const std::string& fileName, 
				   std::vector<std::unique_ptr<GameObject>>& gameObjects,
				   GameObject*& selectedObject
	);
	
private:
	SceneContext* context_ = nullptr;
	std::filesystem::path currentDirectory_ = "Resources";

	// セーブ/ロードUI用のバッファと選択インデックス
	char saveFileName_[128] = "defaultScene.json";
	int selectedSceneFileIndex_ = 0;
};