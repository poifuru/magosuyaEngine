#pragma once
#include "BaseScene.h"
#include "GameObject.h"

class LightManager;
class PostEffectManager;

// ステージセレクト画面（作りは TitleScene と同じ。選択の処理は StageSelectComponent が持つ）
class StageSelectScene : public BaseScene {
public:
	StageSelectScene();
	~StageSelectScene() override;
	void Initialize() override;
	void UpdateGame(const CameraData* cameraData) override;
	void UpdateEdit(const CameraData* cameraData) override;
	void Draw(MyEngine::Rendering::Renderer* renderer) override;
	PostEffectManager* GetPostEffectManager() override { return postEffectManager_.get(); }
private:
	void CleanupObject();
	void UpdateLights();

private:
	std::vector<std::unique_ptr<GameObject>> gameObjects_;
	std::vector<std::unique_ptr<GameObject>> createQueue_;

	std::unique_ptr<LightManager> lightManager_ = nullptr;
	std::unique_ptr<PostEffectManager> postEffectManager_ = nullptr;
};
