#pragma once
#include "BaseScene.h"
#include "GameObject.h"

class LightManager;
class PostEffectManager;

class TitleScene : public BaseScene {
public:
	TitleScene();
	~TitleScene() override;
	void Initialize() override;
	// ゲーム中の更新（スペースキーでPlaySceneへ遷移など）
	void UpdateGame(CameraData* cameraData) override;
	// エディタ編集中の更新
	void UpdateEdit(CameraData* cameraData) override;
	void Draw(MyEngine::Rendering::Renderer* renderer) override;
	PostEffectManager* GetPostEffectManager() override { return postEffectManager_.get(); }
private:
	void CleanupObject();

private:
	std::vector<std::unique_ptr<GameObject>> gameObjects_;
	std::vector<std::unique_ptr<GameObject>> createQueue_;

	GameObject* selectedObject_ = nullptr;

	std::unique_ptr<LightManager> lightManager_ = nullptr;
	std::unique_ptr<PostEffectManager> postEffectManager_ = nullptr;
};