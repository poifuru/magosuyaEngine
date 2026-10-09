#pragma once
#include "BaseScene.h"
#include "GameObject.h"

class LightManager;
class PostEffectManager;
class TutorialManager;

class PlayScene : public BaseScene {
public:
	PlayScene();
	~PlayScene() override;

	void Initialize() override;

	// 更新(ゲーム中)
	void UpdateGame(const CameraData* cameraData) override;

	// 更新(編集中)
	void UpdateEdit(const CameraData* cameraData) override;

	void Draw(MyEngine::Rendering::Renderer* renderer) override;

	PostEffectManager* GetPostEffectManager() override { return postEffectManager_.get(); }

	// ポーズ（一時停止）機能
	void TogglePause();
	bool IsPaused() const { return isPaused_; }

private:
	void CleanupObject();
	void DrawPauseMenu();

private:
	// 全てのGameObject
	std::vector<std::unique_ptr<GameObject>> gameObjects_;
	// 追加待ちのオブジェクトを一時的に溜めるリスト
	std::vector<std::unique_ptr<GameObject>> createQueue_;

	// 選択中のGameObject
	GameObject* selectedObject_ = nullptr;

	std::unique_ptr<LightManager> lightManager_ = nullptr;
	std::unique_ptr<PostEffectManager> postEffectManager_ = nullptr;

	bool isPaused_ = false; // 一時停止フラグ

#ifdef USEIMGUI
	bool isDebugMode_ = true;
#endif
};
