#pragma once
#include "BaseScene.h"
#include "GameObject.h"

class LightManager;
class PostEffectManager;
class LevelEditor;
class TutorialManager;

class PlayScene : public BaseScene {
public:
	PlayScene();
	~PlayScene() override;

	void Initialize() override;
	void Update(CameraData* cameraData) override;
	void Draw(MyEngine::Rendering::Renderer* renderer) override;

	PostEffectManager* GetPostEffectManager() override { return postEffectManager_.get(); }

private:
	void CleanupObject();

private:
	// 全てのGameObject
	std::vector<std::unique_ptr<GameObject>> gameObjects_;
	// 追加待ちのオブジェクトを一時的に溜めるリスト
	std::vector<std::unique_ptr<GameObject>> createQueue_;

	// 選択中のGameObject
	GameObject* selectedObject_ = nullptr;

	std::unique_ptr<LightManager> lightManager_ = nullptr;
	std::unique_ptr<PostEffectManager> postEffectManager_ = nullptr;
	std::unique_ptr<TutorialManager> tutorialManager_ = nullptr;

#ifdef USEIMGUI
	// エディタインスタンス
	std::unique_ptr<LevelEditor> levelEditor_ = nullptr;
	bool isDebugMode_ = true;
#endif
};
