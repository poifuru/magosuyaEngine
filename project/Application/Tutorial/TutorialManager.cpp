#include "PCH.h"
#include "TutorialManager.h"
#include "BaseScene.h"
#include "GameObject.h"
#include "TextDrawerComponent.h"
#include "InputManager.h"
#include "RawInput.h"
#include "PlayerComponent.h"
#include "BirdEnemyComponent.h"
#include "FishEnemyComponent.h"

void TutorialManager::Initialize(BaseScene* scene) {
	scene_ = scene;
	currentStep_ = TutorialStep::Move;
	stepTimer_ = 0.0f;
	isEnemySpawned_ = false;
	isPlayerPosCaptured_ = false;

	SetupTextObject();
}

void TutorialManager::SetupTextObject() {
	if (!scene_) return;
	SceneContext* context = scene_->GetContext();
	if (!context || !context->gameObjects) return;

	// 画面上のUI表示用テキストオブジェクトを作成
	auto textObj = std::make_unique<GameObject>(context, "TutorialText");
	textObj->Initialize();

	textDrawer_ = textObj->AddComponent<TextDrawerComponent>();
	textDrawer_->Initialize();
	textDrawer_->SetText("WASD TO MOVE");
	textDrawer_->SetPosition({ 640.0f, 150.0f });
	textDrawer_->SetSize({ 24.0f, 48.0f });
	textDrawer_->SetColor({ 1.0f, 1.0f, 0.0f, 1.0f }); // 黄色
	textDrawer_->SetAlignment(TextDrawerComponent::Alignment::Center);

	textObject_ = textObj.get();
	context->gameObjects->push_back(std::move(textObj));
}

void TutorialManager::SpawnTutorialEnemy() {
	if (!scene_ || isEnemySpawned_) return;
	SceneContext* context = scene_->GetContext();
	if (!context || !context->gameObjects) return;

	// チュートリアル用の敵（BirdEnemy）を生成
	auto enemyObj = std::make_unique<GameObject>(context, "TutorialEnemy");
	enemyObj->Initialize();

	// プレイヤーの前方に配置
	Vector3 spawnPos = { 0.0f, 2.0f, 10.0f };
	if (context->activeGameObjects) {
		for (auto& obj : *context->activeGameObjects) {
			if (obj && obj->GetComponent<PlayerComponent>()) {
				spawnPos = obj->GetTransform().translate + Vector3{ 0.0f, 2.0f, 15.0f };
				break;
			}
		}
	}
	enemyObj->GetTransform().translate = spawnPos;

	auto enemyComp = enemyObj->AddComponent<BirdEnemyComponent>();
	enemyComp->Initialize();

	enemyObject_ = enemyObj.get();
	context->gameObjects->push_back(std::move(enemyObj));
	isEnemySpawned_ = true;
}

void TutorialManager::Update() {
	if (!scene_) return;

	InputManager* input = InputManager::GetInstance();
	RawInput* rawInput = (input != nullptr) ? input->GetRawInput() : nullptr;

	switch (currentStep_) {
	case TutorialStep::Move:
	{
		bool movedInput = false;
		if (rawInput != nullptr) {
			if (rawInput->Push('W') || rawInput->Push('A') || rawInput->Push('S') || rawInput->Push('D') ||
				rawInput->Push(VK_UP) || rawInput->Push(VK_LEFT) || rawInput->Push(VK_DOWN) || rawInput->Push(VK_RIGHT)) {
				movedInput = true;
			}
		}

		if (movedInput) {
			currentStep_ = TutorialStep::KillEnemy;
			if (textDrawer_ != nullptr) {
				textDrawer_->SetText("ATTACK TO KILL ENEMY");
				textDrawer_->SetColor({ 1.0f, 0.4f, 0.4f, 1.0f }); // 赤色
			}
			SpawnTutorialEnemy();
		}
		break;
	}
	case TutorialStep::KillEnemy:
	{
		bool enemyDefeated = false;

		if (enemyObject_ != nullptr) {
			auto bird = enemyObject_->GetComponent<BirdEnemyComponent>();
			if (bird != nullptr && bird->IsDead()) {
				enemyDefeated = true;
			}
			auto fish = enemyObject_->GetComponent<FishEnemyComponent>();
			if (fish != nullptr && fish->IsDead()) {
				enemyDefeated = true;
			}
		} else {
			// オブジェクト自体が削除された場合もクリア扱い
			enemyDefeated = true;
		}

		if (enemyDefeated) {
			currentStep_ = TutorialStep::Complete;
			stepTimer_ = 0.0f;
			if (textDrawer_ != nullptr) {
				textDrawer_->SetText("TUTORIAL CLEAR!");
				textDrawer_->SetColor({ 0.4f, 1.0f, 0.4f, 1.0f }); // 緑色
			}
		}
		break;
	}
	case TutorialStep::Complete:
	{
		stepTimer_ += 1.0f / 60.0f;
		if (stepTimer_ >= 3.0f) {
			if (textDrawer_ != nullptr) {
				textDrawer_->SetText(""); // メッセージ非表示
			}
		}
		break;
	}
	}
}
