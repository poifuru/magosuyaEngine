#include "PCH.h"
#include "PlayerHealth.h"
#include "GameObject.h"
#include "MeshRendererComponent.h"
#include "CameraOrganizer.h"
#include "../../../../Engine/Editor/ParticleEditor/ParticleSpawner.h"
#include <algorithm>

void PlayerHealth::Initialize() {
	hp_ = 5;
	maxHp_ = 5;
	invincibilityTimer_ = 0.0f;
	invincibilityDuration_ = 1.5f;
	isDead_ = false;
}

void PlayerHealth::Update(GameObject* gameObject) {
	if (!gameObject) return;

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

void PlayerHealth::ImGui() {
#ifdef USEIMGUI
	ImGui::Text("--- Player HP Status ---");
	ImGui::DragInt("HP", &hp_, 1, 0, maxHp_);
	ImGui::DragInt("Max HP", &maxHp_, 1, 1, 100);
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
}
