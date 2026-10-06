#include "PCH.h"
#include "FishEnemyComponent.h"
#include "GameObject.h"
#include "MeshRendererComponent.h"
#include "../../../../Engine/Editor/ParticleEditor/ParticleSpawner.h"
#include "GameDirectorComponent.h"
#include "BaseScene.h"
#include "RenderingModel.h"
#include "PlayerComponent.h"
#include "MathFunction.h"

namespace {
	float LerpAngle(float a, float b, float t) {
		float diff = b - a;
		while (diff < -3.14159265f) diff += 6.2831853f;
		while (diff >  3.14159265f) diff -= 6.2831853f;
		return a + diff * t;
	}

	Vector3 LerpEuler(const Vector3& current, const Vector3& target, float t) {
		return {
			LerpAngle(current.x, target.x, t),
			LerpAngle(current.y, target.y, t),
			LerpAngle(current.z, target.z, t)
		};
	}
}

void FishEnemyComponent::Initialize() {
	if (isInitialized_) return;
	isInitialized_ = true;

	if (gameObject_) {
		startPos_ = gameObject_->GetTransform().translate; // 配置された初期位置を開始地点にする
		waterSurfaceY_ = startPos_.y; // 初期配置の高さを水面とする
	}

	hp_ = 3;
	maxHp_ = 3;
	invincibilityTimer_ = 0.0f;
	invincibilityDuration_ = 0.15f;

	moveRange_ = 10.0f;
	speed_ = 5.0f;
	direction_ = 1.0f;

	state_ = FishState::Submerge;
	stateTimer_ = 0.0f;
	swimPhase_ = 0.0f;
	submergeDuration_ = 3.0f;
	jumpPowerY_ = 15.0f;
	jumpPowerXZ_ = 10.0f;
	gravity_ = -25.0f;
	rotLerpSpeed_ = 8.0f;
}

void FishEnemyComponent::Update() {
	if (!gameObject_) return;

	// 被弾無敵タイマー更新と赤色点滅
	if (invincibilityTimer_ > 0.0f) {
		invincibilityTimer_ -= Time::GetDeltaTime();
		if (invincibilityTimer_ < 0.0f) invincibilityTimer_ = 0.0f;

		bool flash = (static_cast<int>(invincibilityTimer_ * 15.0f) % 2 == 0);
		if (auto* mesh = gameObject_->GetComponent<MeshRendererComponent>()) {
			mesh->SetColor({ 1.0f, flash ? 0.3f : 1.0f, flash ? 0.3f : 1.0f, 1.0f });
		}
	} else if (!isDead_) {
		if (auto* mesh = gameObject_->GetComponent<MeshRendererComponent>()) {
			mesh->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		}
	}

	// 死亡演出の更新
	if (isDead_) {
		deathTimer_ += Time::GetDeltaTime();
		const float kDeathDuration = 1.0f; // 1.0秒で消滅
		float progress = deathTimer_ / kDeathDuration;
		if (progress >= 1.0f) {
			gameObject_->Destroy();
			return;
		}

		// スケールアウト
		float scaleFactor = 1.0f - progress;
		gameObject_->GetTransform().scale = {
			originalScale_.x * scaleFactor,
			originalScale_.y * scaleFactor,
			originalScale_.z * scaleFactor
		};

		// フェードアウト
		if (auto* mesh = gameObject_->GetComponent<MeshRendererComponent>()) {
			mesh->SetBlendMode(MyEngine::Rendering::BlendModeType::Alpha);
			mesh->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f - progress });
		}
		return;
	}

	// 基準となるプレイヤーの位置を探す
	Vector3 playerPos = { 0.0f, 0.0f, 0.0f };
	bool foundPlayer = false;
	auto* context = gameObject_->GetContext();
	if (context && context->activeGameObjects) {
		for (const auto& obj : *(context->activeGameObjects)) {
			if (obj->GetName() == "Player" || obj->GetComponent<PlayerComponent>() != nullptr) {
				playerPos = obj->GetTransform().translate;
				foundPlayer = true;
				break;
			}
		}
	}

	auto& trans = gameObject_->GetTransform();
	Vector3 targetRot = trans.rotate; // デフォルトの目標角度

	switch (state_) {
	case FishState::Submerge: {
		// 水面より少し下に体を沈める
		float targetY = waterSurfaceY_ - 0.5f;
		trans.translate.y += (targetY - trans.translate.y) * 5.0f * Time::GetDeltaTime();

		if (foundPlayer) {
			Vector3 toPlayer = playerPos - trans.translate;
			toPlayer.y = 0.0f; // 水平距離
			float distXZ = Math::Length(toPlayer);

			if (distXZ > 0.1f) {
				Vector3 fwdDir = Math::Normalize(toPlayer);
				// 進行方向に垂直な横向きベクトル (Right Vector)
				Vector3 rightDir = { -fwdDir.z, 0.0f, fwdDir.x };

				// 左右にゆらゆら揺れるサイン波（S字蛇行進行）
				swimPhase_ += speed_ * 1.5f * Time::GetDeltaTime();
				float sway = std::sin(swimPhase_) * 0.8f; // 左右の揺れ幅

				// 接近移動ベクトル = (プレイヤー方向への前進) + (左右への波状揺れ)
				Vector3 moveVel = Math::Add(
					Math::Multiply(speed_, fwdDir),
					Math::Multiply(speed_ * sway, rightDir)
				);

				// 移動の適用
				trans.translate += moveVel * Time::GetDeltaTime();

				// 頭（向き）を進行方向に滑らかに向かせる
				if (Math::Length(moveVel) > 0.001f) {
					Vector3 moveDirNorm = Math::Normalize(moveVel);
					targetRot.y = std::atan2(moveDirNorm.x, moveDirNorm.z);
					targetRot.x = 0.0f;
					targetRot.z = 0.0f;
				}
			}

			// プレイヤーが近く（25m以内）にいて一定の潜水時間が経過したら水面ジャンプ攻撃！
			if (distXZ < 25.0f) {
				stateTimer_ += Time::GetDeltaTime();
				if (stateTimer_ >= submergeDuration_) {
					stateTimer_ = 0.0f;
					state_ = FishState::Jump;

					// プレイヤーの方向へ向かって放物線を描いて飛ぶ
					Vector3 dirXZ = Math::Normalize(toPlayer);

					// 滞空時間を計算 (t = -2 * V0y / gravity)
					float airTime = (gravity_ < -0.001f) ? (-2.0f * jumpPowerY_ / gravity_) : 1.0f;

					// プレイヤーにピッタリ届くための水平初速を計算
					float requiredPowerXZ = distXZ / airTime;
					if (requiredPowerXZ > 40.0f) requiredPowerXZ = 40.0f; // 速度上限ガード

					velocity_.x = dirXZ.x * requiredPowerXZ;
					velocity_.z = dirXZ.z * requiredPowerXZ;
					velocity_.y = jumpPowerY_;

					// 水しぶきパーティクル
					ParticleSpawner::SpawnExplosion(context, trans.translate, 5);
				}
			}
		} else {
			// プレイヤーが見つからない場合はその場で優雅に円を描いて泳ぐ
			swimPhase_ += speed_ * 0.5f * Time::GetDeltaTime();
			trans.translate.x += std::cos(swimPhase_) * speed_ * Time::GetDeltaTime();
			trans.translate.z += std::sin(swimPhase_) * speed_ * Time::GetDeltaTime();
			targetRot.y = swimPhase_ + 1.570796f;
		}
		break;
	}
	case FishState::Jump: {
		// 速度を適用
		trans.translate += velocity_ * Time::GetDeltaTime();

		// 重力を適用
		velocity_.y += gravity_ * Time::GetDeltaTime();

		// 進行方向を向かせる目標回転
		if (Math::Length(velocity_) > 0.1f) {
			Vector3 dir = Math::Normalize(velocity_);
			targetRot.y = std::atan2(dir.x, dir.z);
			float xzLen = std::sqrt(dir.x * dir.x + dir.z * dir.z);
			targetRot.x = std::atan2(-dir.y, xzLen);
			targetRot.z = 0.0f;
		}

		// 着水判定
		if (trans.translate.y <= waterSurfaceY_ && velocity_.y < 0.0f) {
			state_ = FishState::Submerge;
			stateTimer_ = 0.0f;

			// 着水時のしぶきエフェクト
			ParticleSpawner::SpawnExplosion(context, trans.translate, 5);

			// 着水した位置を新しい往復開始位置にする
			startPos_ = trans.translate;
			startPos_.y = waterSurfaceY_;
		}
		break;
	}
	}

	// 角度を最短ルートで滑らかに補間する（360度大回転スピン防止）
	float rotLerpRate = rotLerpSpeed_ * Time::GetDeltaTime();
	if (rotLerpRate > 1.0f) rotLerpRate = 1.0f;
	trans.rotate = LerpEuler(trans.rotate, targetRot, rotLerpRate);
}

void FishEnemyComponent::ImGui() {
	ImGui::DragFloat3("Start Pos", &startPos_.x, 0.1f);
	ImGui::DragFloat("Move Range", &moveRange_, 0.1f, 0.0f, 100.0f);
	ImGui::DragFloat("Speed", &speed_, 0.1f, 0.0f, 50.0f);
	ImGui::DragInt("HP", &hp_, 1, 0, maxHp_);
	ImGui::DragInt("Max HP", &maxHp_, 1, 1, 50);
	ImGui::Separator();
	ImGui::DragFloat("Submerge Duration", &submergeDuration_, 0.1f, 0.0f, 20.0f);
	ImGui::DragFloat("Jump Power Y", &jumpPowerY_, 0.5f, 0.0f, 100.0f);
	ImGui::DragFloat("Jump Power XZ (Default)", &jumpPowerXZ_, 0.5f, 0.0f, 100.0f);
	ImGui::DragFloat("Gravity", &gravity_, 0.5f, -100.0f, 0.0f);
	ImGui::DragFloat("Water Surface Y", &waterSurfaceY_, 0.1f, -50.0f, 50.0f);
	ImGui::DragInt("Energy Reward", &energyReward_, 1, 0, 100);
	ImGui::DragFloat("Rotation Lerp Speed", &rotLerpSpeed_, 0.1f, 0.1f, 50.0f);

	const char* stateStr = "Unknown";
	if (state_ == FishState::Submerge) stateStr = "Submerge";
	else if (state_ == FishState::Jump) stateStr = "Jump";
	ImGui::Text("Current State: %s", stateStr);
}

void FishEnemyComponent::Serialize(json& j) const {
	j["type"] = "FishEnemyComponent";
	j["startPos"] = { startPos_.x, startPos_.y, startPos_.z };
	j["moveRange"] = moveRange_;
	j["speed"] = speed_;
	j["direction"] = direction_;
	j["hp"] = hp_;
	j["maxHp"] = maxHp_;
	j["energyReward"] = energyReward_;
	j["submergeDuration"] = submergeDuration_;
	j["jumpPowerY"] = jumpPowerY_;
	j["jumpPowerXZ"] = jumpPowerXZ_;
	j["gravity"] = gravity_;
	j["waterSurfaceY"] = waterSurfaceY_;
	j["rotLerpSpeed"] = rotLerpSpeed_;
}

void FishEnemyComponent::Deserialize(const json& j) {
	isInitialized_ = true;
	if (j.contains("startPos")) {
		startPos_ = { j["startPos"][0], j["startPos"][1], j["startPos"][2] };
	}
	if (j.contains("moveRange")) moveRange_ = j["moveRange"];
	if (j.contains("speed")) speed_ = j["speed"];
	if (j.contains("direction")) direction_ = j["direction"];
	if (j.contains("hp")) hp_ = j["hp"];
	if (j.contains("maxHp")) maxHp_ = j["maxHp"];
	if (j.contains("energyReward")) energyReward_ = j["energyReward"];
	if (j.contains("submergeDuration")) submergeDuration_ = j["submergeDuration"];
	if (j.contains("jumpPowerY")) jumpPowerY_ = j["jumpPowerY"];
	if (j.contains("jumpPowerXZ")) jumpPowerXZ_ = j["jumpPowerXZ"];
	if (j.contains("gravity")) gravity_ = j["gravity"];
	if (j.contains("waterSurfaceY")) waterSurfaceY_ = j["waterSurfaceY"];
	if (j.contains("rotLerpSpeed")) rotLerpSpeed_ = j["rotLerpSpeed"];
}

void FishEnemyComponent::TakeDamage(int damage) {
	if (isDead_ || invincibilityTimer_ > 0.0f) return;

	hp_ -= damage;
	invincibilityTimer_ = invincibilityDuration_;

	// 被弾エフェクト
	if (gameObject_) {
		ParticleSpawner::SpawnExplosion(gameObject_->GetContext(), gameObject_->GetTransform().translate, 6);
	}

	if (hp_ <= 0) {
		hp_ = 0;
		OnDead();
	}
}

void FishEnemyComponent::OnDead() {
	if (isDead_) return;
	isDead_ = true;
	deathTimer_ = 0.0f;
	if (gameObject_) {
		originalScale_ = gameObject_->GetTransform().scale;

		// 被弾位置に爆発パーティクルを生成
		ParticleSpawner::SpawnExplosion(gameObject_->GetContext(), gameObject_->GetTransform().translate, 15);

		// プレイヤーへの電力還元とGameDirectorへの撃破通知
		if (gameObject_->GetContext() && gameObject_->GetContext()->activeGameObjects) {
			for (auto& obj : *(gameObject_->GetContext()->activeGameObjects)) {
				if (auto* playerComp = obj->GetComponent<PlayerComponent>()) {
					playerComp->Heal(energyReward_);
				}
				if (auto* director = obj->GetComponent<GameDirectorComponent>()) {
					director->NotifyEnemyDead();
				}
			}
		}
	}
}