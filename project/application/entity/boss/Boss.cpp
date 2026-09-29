#include "PCH.h"
#include "Boss.h"
#include "GameObject.h"
#include "MeshRendererComponent.h"
#include "ColliderComponent.h"
#include "PlayerComponent.h"
#include "CameraOrganizer.h"
#include "BaseScene.h"
#include "MathFunction.h"
#include "../../../../Engine/Editor/ParticleEditor/ParticleSpawner.h"
#include <algorithm>
#include <cmath>

void BossComponent::Initialize() {
	if (isInitialized_) return;
	isInitialized_ = true;

	hp_ = 50;
	maxHp_ = 50;
	isDead_ = false;
	invincibilityTimer_ = 0.0f;
	swimTime_ = 0.0f;
	deathTimer_ = 0.0f;

	// コライダーの自動設定（巨大魚サイズ）
	if (gameObject_) {
		auto* collider = gameObject_->GetComponent<ColliderComponent>();
		if (!collider) {
			collider = gameObject_->AddComponent<ColliderComponent>();
		}
		if (collider) {
			collider->SetRadius(8.0f);
		}
	}
}

void BossComponent::Update() {
	if (!gameObject_ || isDebugMode_) return;

	auto& myTransform = gameObject_->GetTransform();

	// 死亡時アニメーション
	if (isDead_) {
		deathTimer_ += Time::GetDeltaTime();
		myTransform.translate.y -= 2.5f * Time::GetDeltaTime(); // ゆっくり海底へ沈む
		myTransform.rotate.z += 0.4f * Time::GetDeltaTime();     // 横倒しになる

		// 連続爆発エフェクト
		static float deathFxTimer = 0.0f;
		deathFxTimer += Time::GetDeltaTime();
		if (deathFxTimer >= 0.25f && deathTimer_ < 3.0f) {
			deathFxTimer = 0.0f;
			if (gameObject_->GetContext()) {
				ParticleSpawner::SpawnExplosion(gameObject_->GetContext(), myTransform.translate, 12);
			}
		}
		return;
	}

	swimTime_ += Time::GetDeltaTime();

	// 無敵タイマー更新と赤色点滅
	if (invincibilityTimer_ > 0.0f) {
		invincibilityTimer_ -= Time::GetDeltaTime();
		if (invincibilityTimer_ < 0.0f) invincibilityTimer_ = 0.0f;

		bool flash = (static_cast<int>(invincibilityTimer_ * 15.0f) % 2 == 0);
		if (auto* mesh = gameObject_->GetComponent<MeshRendererComponent>()) {
			mesh->SetColor({ 1.0f, flash ? 0.2f : 0.6f, flash ? 0.2f : 0.8f, 1.0f });
		}
	} else {
		if (auto* mesh = gameObject_->GetComponent<MeshRendererComponent>()) {
			mesh->SetColor({ 0.6f, 0.7f, 0.95f, 1.0f });
		}
	}

	// プレイヤーの探索
	Vector3 playerPos = { 0.0f, -30.0f, 0.0f };
	bool foundPlayer = false;
	if (gameObject_->GetContext() && gameObject_->GetContext()->activeGameObjects) {
		for (const auto& obj : *(gameObject_->GetContext()->activeGameObjects)) {
			if (obj->GetName() == "Player" || obj->GetComponent<PlayerComponent>() != nullptr) {
				playerPos = obj->GetTransform().translate;
				foundPlayer = true;
				break;
			}
		}
	}

	// プレイヤー方向への旋回と優雅な遊泳
	Vector3 toPlayer = Math::Subtract(playerPos, myTransform.translate);
	float distToPlayer = Math::Length(toPlayer);

	if (distToPlayer > 0.001f) {
		Vector3 dir = Math::Normalize(toPlayer);

		// 左右旋回（Yaw）
		float targetYaw = std::atan2(dir.x, dir.z);
		float currentYaw = myTransform.rotate.y;
		float diffYaw = targetYaw - currentYaw;
		while (diffYaw < -3.14159265f) diffYaw += 6.2831853f;
		while (diffYaw > 3.14159265f) diffYaw -= 6.2831853f;
		myTransform.rotate.y += diffYaw * turnSpeed_ * Time::GetDeltaTime();

		// 上下旋回（Pitch）
		float xzLen = std::sqrt(dir.x * dir.x + dir.z * dir.z);
		float targetPitch = std::atan2(-dir.y, xzLen);
		float currentPitch = myTransform.rotate.x;
		float diffPitch = targetPitch - currentPitch;
		while (diffPitch < -3.14159265f) diffPitch += 6.2831853f;
		while (diffPitch > 3.14159265f) diffPitch -= 6.2831853f;
		myTransform.rotate.x += diffPitch * turnSpeed_ * Time::GetDeltaTime();
	}

	// 正面方向ベクトルを計算
	float cy = std::cos(myTransform.rotate.y);
	float sy = std::sin(myTransform.rotate.y);
	float cx = std::cos(myTransform.rotate.x);
	float sx = std::sin(myTransform.rotate.x);
	Vector3 forward = { sy * cx, -sx, cy * cx };
	if (Math::Length(forward) > 0.001f) {
		forward = Math::Normalize(forward);
	}

	// プレイヤーとの間合いに応じた移動速度調整
	float currentSpeed = (distToPlayer > targetDistance_) ? moveSpeed_ : moveSpeed_ * 0.4f;

	myTransform.translate.x += forward.x * currentSpeed * Time::GetDeltaTime();
	myTransform.translate.y += forward.y * currentSpeed * Time::GetDeltaTime();
	myTransform.translate.z += forward.z * currentSpeed * Time::GetDeltaTime();

	// 深海魚のうねり遊泳（上下とロールのゆらぎ）
	myTransform.translate.y += std::sin(swimTime_ * bobbingSpeed_) * bobbingAmount_ * Time::GetDeltaTime();
	myTransform.rotate.z = std::sin(swimTime_ * bobbingSpeed_ * 1.5f) * 0.08f;

	// 水深制限（水面下〜海底）
	if (myTransform.translate.y > -5.0f) myTransform.translate.y = -5.0f;
	if (myTransform.translate.y < -90.0f) myTransform.translate.y = -90.0f;
}

void BossComponent::TakeDamage(int damage) {
	if (isDead_ || invincibilityTimer_ > 0.0f) return;

	hp_ -= damage;
	invincibilityTimer_ = invincibilityDuration_;

	// 被弾エフェクトとカメラシェイク
	if (gameObject_) {
		ParticleSpawner::SpawnExplosion(gameObject_->GetContext(), gameObject_->GetTransform().translate, 10);
	}
	CameraOrganizer::GetInstance()->Shake(0.2f, 0.35f);

	if (hp_ <= 0) {
		hp_ = 0;
		OnDead();
	}
}

void BossComponent::OnDead() {
	if (isDead_) return;
	isDead_ = true;
	deathTimer_ = 0.0f;

	// 巨大爆発エフェクトと強シェイク
	if (gameObject_ && gameObject_->GetContext()) {
		ParticleSpawner::SpawnExplosion(gameObject_->GetContext(), gameObject_->GetTransform().translate, 30);
	}
	CameraOrganizer::GetInstance()->Shake(0.8f, 0.6f);
}

void BossComponent::ImGui() {
#ifdef USEIMGUI
	ImGui::Text("--- Boss Status ---");
	ImGui::DragInt("HP", &hp_, 1, 0, maxHp_);
	ImGui::DragInt("Max HP", &maxHp_, 1, 1, 1000);

	float hpRatio = maxHp_ > 0 ? static_cast<float>(hp_) / static_cast<float>(maxHp_) : 0.0f;
	char hpBuf[32];
	sprintf_s(hpBuf, "Boss HP: %d / %d", hp_, maxHp_);
	ImGui::ProgressBar(hpRatio, ImVec2(-1, 0), hpBuf);

	if (ImGui::Button("Test Damage (5 HP)")) {
		TakeDamage(5);
	}

	ImGui::Separator();
	ImGui::DragFloat("Move Speed", &moveSpeed_, 0.1f, 0.0f, 20.0f);
	ImGui::DragFloat("Turn Speed", &turnSpeed_, 0.05f, 0.1f, 5.0f);
	ImGui::DragFloat("Target Distance", &targetDistance_, 0.5f, 5.0f, 100.0f);
	ImGui::DragFloat("Bobbing Speed", &bobbingSpeed_, 0.1f, 0.1f, 10.0f);
	ImGui::DragFloat("Bobbing Amount", &bobbingAmount_, 0.1f, 0.0f, 10.0f);
#endif
}

void BossComponent::Serialize(json& j) const {
	j["type"] = "BossComponent";
	j["hp"] = hp_;
	j["maxHp"] = maxHp_;
	j["moveSpeed"] = moveSpeed_;
	j["turnSpeed"] = turnSpeed_;
	j["targetDistance"] = targetDistance_;
}

void BossComponent::Deserialize(const json& j) {
	isInitialized_ = true;
	if (j.contains("hp")) hp_ = j["hp"];
	if (j.contains("maxHp")) maxHp_ = j["maxHp"];
	if (j.contains("moveSpeed")) moveSpeed_ = j["moveSpeed"];
	if (j.contains("turnSpeed")) turnSpeed_ = j["turnSpeed"];
	if (j.contains("targetDistance")) targetDistance_ = j["targetDistance"];
}