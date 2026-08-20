#pragma once
#include "MathTypes.h"
#include <string>

class BaseScene;
class GameObject;
class TextDrawerComponent;

enum class TutorialStep {
	Move,       // 移動練習
	KillEnemy,  // 敵撃破練習
	Complete    // 完了
};

class TutorialManager {
public:
	TutorialManager() = default;
	~TutorialManager() = default;

	void Initialize(BaseScene* scene);
	void Update();

	TutorialStep GetCurrentStep() const { return currentStep_; }
	bool IsComplete() const { return currentStep_ == TutorialStep::Complete; }

private:
	void SetupTextObject();
	void SpawnTutorialEnemy();

private:
	BaseScene* scene_ = nullptr;
	GameObject* textObject_ = nullptr;
	TextDrawerComponent* textDrawer_ = nullptr;
	GameObject* enemyObject_ = nullptr;

	TutorialStep currentStep_ = TutorialStep::Move;
	float stepTimer_ = 0.0f;
	bool isEnemySpawned_ = false;
	Vector3 playerStartPos_{};
	bool isPlayerPosCaptured_ = false;
};
