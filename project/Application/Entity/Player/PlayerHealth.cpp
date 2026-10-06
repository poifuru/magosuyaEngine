#include "PCH.h"
#include "PlayerHealth.h"
#include "GameObject.h"
#include "MeshRendererComponent.h"
#include "SpriteComponent.h"
#include "CameraOrganizer.h"
#include "../../../../Engine/Editor/ParticleEditor/ParticleSpawner.h"
#include <algorithm>

PlayerHealth::~PlayerHealth() {
	if (hpBorderObj_) hpBorderObj_->Destroy();
	if (hpBgObj_) hpBgObj_->Destroy();
	if (hpBarObj_) hpBarObj_->Destroy();
}

void PlayerHealth::Initialize() {
	hp_ = 100;
	maxHp_ = 100;
	invincibilityTimer_ = 0.0f;
	invincibilityDuration_ = 1.5f;
	isDead_ = false;

	uiCreated_ = false;
	hpBorderObj_ = nullptr;
	hpBgObj_ = nullptr;
	hpBarObj_ = nullptr;
}

void PlayerHealth::Update(GameObject* gameObject) {
	if (!gameObject) return;

	// UI生成（未生成または破棄されていた場合に安全に再生成）
	if (!uiCreated_ || !hpBarObj_ || hpBarObj_->IsDead()) {
		uiCreated_ = false;
		CreateUI(gameObject);
	}

	// UIのサイズ・色を現在HPに合わせて更新
	UpdateUI();

	// 無敵タイマーと被弾点滅演出
	if (invincibilityTimer_ > 0.0f) {
		invincibilityTimer_ -= Time::GetDeltaTime();
		if (invincibilityTimer_ < 0.0f) invincibilityTimer_ = 0.0f;

		// 赤く点滅
		bool flashRed = (static_cast<int>(invincibilityTimer_ * 10.0f) % 2 == 0);
		if (auto* mesh = gameObject->GetComponent<MeshRendererComponent>()) {
			mesh->SetColor({ 1.0f, flashRed ? 0.3f : 1.0f, flashRed ? 0.3f : 1.0f, 1.0f });
		}
	}
	else {
		if (auto* mesh = gameObject->GetComponent<MeshRendererComponent>()) {
			mesh->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		}
	}
}

void PlayerHealth::TakeDamage(GameObject* gameObject, int damage) {
	if (isDead_ || invincibilityTimer_ > 0.0f) return;

	hp_ -= damage;
	invincibilityTimer_ = invincibilityDuration_;

	// 被弾エフェクト
	if (gameObject) {
		ParticleSpawner::SpawnExplosion(gameObject->GetContext(), gameObject->GetTransform().translate, 10);
	}

	// カメラシェイク
	CameraOrganizer::GetInstance()->Shake(0.35f, 0.4f);

	if (hp_ <= 0) {
		hp_ = 0;
		isDead_ = true;
	}
}

bool PlayerHealth::ConsumeHealth(int amount) {
	// 最低1は電力を残してリロードによる自滅を防ぐ（背水のリロード：電力が足りなくても1残して成立させる）
	if (isDead_ || hp_ <= 1) return false;
	int consume = (hp_ > amount) ? amount : (hp_ - 1);
	hp_ -= consume;
	return true;
}

void PlayerHealth::Heal(int amount) {
	if (isDead_) return;
	hp_ = std::min(maxHp_, hp_ + amount);
}

void PlayerHealth::ImGui() {
#ifdef USEIMGUI
	ImGui::Text("--- Player HP Status ---");
	ImGui::DragInt("HP", &hp_, 1, 0, maxHp_);
	ImGui::DragInt("Max HP", &maxHp_, 1, 1, 500);
	float hpRatio = maxHp_ > 0 ? static_cast<float>(hp_) / static_cast<float>(maxHp_) : 0.0f;
	char hpBuf[32];
	sprintf_s(hpBuf, "HP: %d / %d", hp_, maxHp_);
	ImGui::ProgressBar(hpRatio, ImVec2(-1, 0), hpBuf);

	if (ImGui::Button("Test Damage (1 HP)")) {
		// ※ボタンによるテスト用（gameObjectが必要なエフェクト等を除くHP減算のみ、または直接減算）
		hp_ -= 1;
		if (hp_ <= 0) {
			hp_ = 0;
			isDead_ = true;
		}
	}
	ImGui::DragFloat("Invincibility Duration", &invincibilityDuration_, 0.1f, 0.0f, 10.0f);
#endif
}

void PlayerHealth::Serialize(json& j) const {
	j["hp"] = hp_;
	j["maxHp"] = maxHp_;
}

void PlayerHealth::Deserialize(const json& j) {
	if (j.contains("hp")) hp_ = j["hp"];
	if (j.contains("maxHp")) maxHp_ = j["maxHp"];
	// 旧セーブデータや未調整値（maxHp < 100）の場合はデフォルトの100に底上げ補正
	if (maxHp_ < 100) {
		maxHp_ = 100;
		hp_ = 100;
	}
}

void PlayerHealth::CreateUI(GameObject* gameObject) {
	if (!gameObject) return;
	auto* context = gameObject->GetContext();
	if (!context || !context->gameObjects) return;

	// 外枠（黒〜ダークグレー）
	auto borderObj = std::make_unique<GameObject>(context, "PlayerHP_Border");
	auto* borderSprite = borderObj->AddComponent<SpriteComponent>();
	borderSprite->SetTexture("Resources/human/white.png");
	borderSprite->SetAnchorPoint({ 0.0f, 0.0f });
	borderSprite->SetPosition({ 48.0f, 638.0f });
	borderSprite->SetSize({ 244.0f, 26.0f });
	borderSprite->SetColor({ 0.05f, 0.05f, 0.08f, 0.9f });
	borderSprite->SetLayer(3);
	borderObj->Initialize();
	borderObj->SetSerializable(false);
	hpBorderObj_ = borderObj.get();
	context->gameObjects->push_back(std::move(borderObj));

	// ゲージ背景（暗い赤系：減少したHPの下地）
	auto bgObj = std::make_unique<GameObject>(context, "PlayerHP_BG");
	auto* bgSprite = bgObj->AddComponent<SpriteComponent>();
	bgSprite->SetTexture("Resources/human/white.png");
	bgSprite->SetAnchorPoint({ 0.0f, 0.0f });
	bgSprite->SetPosition({ 50.0f, 640.0f });
	bgSprite->SetSize({ 240.0f, 22.0f });
	bgSprite->SetColor({ 0.25f, 0.08f, 0.08f, 0.85f });
	bgSprite->SetLayer(4);
	bgObj->Initialize();
	bgObj->SetSerializable(false);
	hpBgObj_ = bgObj.get();
	context->gameObjects->push_back(std::move(bgObj));

	// HPゲージ本体（現在体力）
	auto barObj = std::make_unique<GameObject>(context, "PlayerHP_Bar");
	auto* barSprite = barObj->AddComponent<SpriteComponent>();
	barSprite->SetTexture("Resources/human/white.png");
	barSprite->SetAnchorPoint({ 0.0f, 0.0f });
	barSprite->SetPosition({ 50.0f, 640.0f });
	barSprite->SetSize({ 240.0f, 22.0f });
	barSprite->SetColor({ 0.2f, 0.85f, 0.35f, 1.0f });
	barSprite->SetLayer(5);
	barObj->Initialize();
	barObj->SetSerializable(false);
	hpBarObj_ = barObj.get();
	context->gameObjects->push_back(std::move(barObj));

	uiCreated_ = true;
}

void PlayerHealth::UpdateUI() {
	if (!hpBarObj_) return;

	float ratio = 0.0f;
	if (maxHp_ > 0) {
		ratio = static_cast<float>(hp_) / static_cast<float>(maxHp_);
		if (ratio < 0.0f) ratio = 0.0f;
		if (ratio > 1.0f) ratio = 1.0f;
	}

	if (auto* barSprite = hpBarObj_->GetComponent<SpriteComponent>()) {
		// HPゲージの長さを現在HP割合に合わせて伸縮
		barSprite->SetSize({ 240.0f * ratio, 22.0f });

		// 残りHPに応じて色を変化（緑 -> 黄 -> 赤）
		Vector4 barColor;
		if (ratio > 0.5f) {
			barColor = { 0.2f, 0.85f, 0.35f, 1.0f }; // 安全（グリーン）
		} else if (ratio > 0.25f) {
			barColor = { 0.95f, 0.8f, 0.2f, 1.0f };  // 注意（イエロー）
		} else {
			barColor = { 0.95f, 0.25f, 0.25f, 1.0f }; // 危険（レッド）
		}

		// 被弾無敵中の点滅演出（白くピカピカ点滅させて被弾を直感的にアピール）
		if (invincibilityTimer_ > 0.0f) {
			bool flashWhite = (static_cast<int>(invincibilityTimer_ * 15.0f) % 2 == 0);
			if (flashWhite) {
				barColor = { 1.0f, 1.0f, 1.0f, 1.0f };
			}
		}

		barSprite->SetColor(barColor);
	}
}
