#include "PCH.h"
#include "BirdEnemyComponent.h"
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

void BirdEnemyComponent::Initialize() {
	if (isInitialized_) return;
	isInitialized_ = true;

	if (gameObject_) {
		center_ = gameObject_->GetTransform().translate; // 配置された初期位置を中心にする
		patrolCenter_ = center_;
	}

	radius_ = 18.0f;
	currentRadius_ = radius_;
	speed_ = 1.5f;
	angle_ = 0.0f;

	hp_ = 2;
	maxHp_ = 2;
	invincibilityTimer_ = 0.0f;
	invincibilityDuration_ = 0.15f;

	formationIndex_ = 0;
	totalBirds_ = 1;

	state_ = BirdState::Patrol;
	stateTimer_ = 0.0f;
	circleDuration_ = 7.5f;
	detectionRange_ = 25.0f;
	diveSpeed_ = 25.0f;
	ascentSpeed_ = 12.0f;
	baseHeight_ = 6.0f;
	rotLerpSpeed_ = 8.0f; // 補間速度の初期値
}

void BirdEnemyComponent::Update() {
	if (!gameObject_) return;

	// 被弾無敵タイマー更新と赤色点滅
	if (invincibilityTimer_ > 0.0f) {
		invincibilityTimer_ -= Time::GetDeltaTime();
		if (invincibilityTimer_ < 0.0f) invincibilityTimer_ = 0.0f;

		bool flash = (static_cast<int>(invincibilityTimer_ * 15.0f) % 2 == 0);
		if (auto* mesh = gameObject_->GetComponent<MeshRendererComponent>()) {
			mesh->SetColor({ 1.0f, flash ? 0.2f : 1.0f, flash ? 0.2f : 1.0f, 1.0f });
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

		// 高度を下げる（落下演出）
		float fallDist = 5.0f * progress;
		gameObject_->GetTransform().translate.y = originalPosition_.y - fallDist;

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
	Vector3 targetRot = trans.rotate; // 現在の回転をデフォルトの目標にする

	switch (state_) {
	case BirdState::Patrol: {
		// 初期位置付近をゆっくりと小旋回して遠方待機する
		angle_ += (speed_ * 0.5f) * Time::GetDeltaTime();
		if (angle_ > 3.14159265f * 2.0f) {
			angle_ -= 3.14159265f * 2.0f;
		}

		Vector3 nextPos = {
			patrolCenter_.x + 5.0f * std::cos(angle_),
			patrolCenter_.y,
			patrolCenter_.z + 5.0f * std::sin(angle_)
		};
		trans.translate = nextPos;

		targetRot.y = -angle_ + 1.570796f;
		targetRot.x = 0.0f;
		targetRot.z = 0.0f;

		// プレイヤーが縄張り範囲（detectionRange_）に入ったら威嚇旋回モード（Circle）へ移行！
		if (foundPlayer) {
			Vector3 diff = playerPos - trans.translate;
			float distToPlayer = Math::Length(diff);
			if (distToPlayer <= detectionRange_) {
				state_ = BirdState::Circle;
				stateTimer_ = 0.0f;

				// center_ をプレイヤーの現在位置へ即時設定（ガクッとするワープの完全防止）
				center_ = { playerPos.x, playerPos.y + baseHeight_, playerPos.z };

				// 旋回移動へスムーズに入るため、現在位置から実効半径と角度を設定
				Vector3 offset = trans.translate - center_;
				offset.y = 0.0f; // 水平距離
				currentRadius_ = Math::Length(offset);
				if (currentRadius_ < 1.0f) currentRadius_ = radius_;
				angle_ = std::atan2(offset.z, offset.x);
			}
		}
		break;
	}
	case BirdState::Circle: {
		// プレイヤーの位置を追従（XZ平面）
		if (foundPlayer) {
			float lerpRate = 5.0f * Time::GetDeltaTime(); // 追従速度を上げてガタつき防止
			center_.x += (playerPos.x - center_.x) * lerpRate;
			center_.z += (playerPos.z - center_.z) * lerpRate;
			center_.y = playerPos.y + baseHeight_;
		}

		// 自転進行角
		float nextAngle = angle_ + speed_ * Time::GetDeltaTime();

		// 隊列（フォーメーション）アラインメント: 複数の鳥が旋回中の場合、リーダー（0番目）を基準に等角度にアラインする
		if (totalBirds_ > 1) {
			if (gameObject_ && gameObject_->GetContext() && gameObject_->GetContext()->activeGameObjects) {
				BirdEnemyComponent* leader = nullptr;
				for (const auto& obj : *(gameObject_->GetContext()->activeGameObjects)) {
					if (auto* bird = obj->GetComponent<BirdEnemyComponent>()) {
						if (!bird->IsDead() && bird->GetState() == BirdState::Circle && bird->GetFormationIndex() == 0) {
							leader = bird;
							break;
						}
					}
				}

				if (leader && leader != this) {
					float leaderNextAngle = leader->GetAngle() + speed_ * Time::GetDeltaTime();
					float targetAngle = leaderNextAngle + (3.14159265f * 2.0f / static_cast<float>(totalBirds_)) * static_cast<float>(formationIndex_);
					
					// 自転後の角度 nextAngle を目標位相 targetAngle に向けてスムーズに統合（ガタつき完全消滅）
					nextAngle = LerpAngle(nextAngle, targetAngle, 2.0f * Time::GetDeltaTime());
				}
			}
		}

		angle_ = nextAngle;
		if (angle_ > 3.14159265f * 2.0f) {
			angle_ -= 3.14159265f * 2.0f;
		}
		if (angle_ < 0.0f) {
			angle_ += 3.14159265f * 2.0f;
		}

		// currentRadius_ を本来の radius_ へ滑らかに引き締める（ガクッとするワープの完全防止）
		currentRadius_ += (radius_ - currentRadius_) * 2.0f * Time::GetDeltaTime();

		// 円の軌道上の座標を計算（実効半径 currentRadius_ を使用）
		Vector3 nextPos = {
			center_.x + currentRadius_ * std::cos(angle_),
			center_.y,
			center_.z + currentRadius_ * std::sin(angle_)
		};
		trans.translate = nextPos;

		// 目標回転の計算 (円旋回時)
		// Yawは進行方向（接線方向）。Pitch, Rollは0.0f。
		targetRot.y = -angle_ + 1.570796f;
		targetRot.x = 0.0f;
		targetRot.z = 0.0f;

		// タイマー更新とフォーメーション連鎖ダイブの判定（1.0秒間隔の連続ウェーブ攻撃）
		stateTimer_ += Time::GetDeltaTime();
		float diveInterval = 1.0f; // 鳥ごとの攻撃待機間隔（秒）
		float totalWaitTime = circleDuration_ + static_cast<float>(formationIndex_) * diveInterval;

		if (stateTimer_ >= totalWaitTime) {
			stateTimer_ = 0.0f;
			if (foundPlayer) {
				state_ = BirdState::Dive;
				diveTarget_ = playerPos;
			}
		}
		break;
	}
	case BirdState::Dive: {
		// 急降下ターゲットに向けて一直線に進む
		Vector3 toTarget = diveTarget_ - trans.translate;
		float dist = Math::Length(toTarget);

		if (dist > 0.1f) {
			Vector3 dir = Math::Normalize(toTarget);
			trans.translate += dir * diveSpeed_ * Time::GetDeltaTime();

			// 水平方向の向きベクトルを記憶（上昇時の通り抜けに使用）
			Vector3 horiz = { dir.x, 0.0f, dir.z };
			if (Math::Length(horiz) > 0.01f) {
				diveHorizDir_ = Math::Normalize(horiz);
			}

			// 進行方向を向く
			targetRot.y = std::atan2(dir.x, dir.z) + 1.570796f;

			// Pitch(上下)はZ軸回転
			float xzLen = std::sqrt(dir.x * dir.x + dir.z * dir.z);
			float pitchAngle = std::atan2(dir.y, xzLen);
			targetRot.z = pitchAngle;
			targetRot.x = 0.0f;
		}

		// ターゲットに到達したか、一定の高度以下に達したら上昇状態へ
		if (dist < 1.5f || trans.translate.y <= diveTarget_.y + 0.2f) {
			state_ = BirdState::Ascent;
			stateTimer_ = 0.0f;
		}
		break;
	}
	case BirdState::Ascent: {
		// Uターンせず、ダイブした水平方向のまま斜め前上に通り抜けながら上昇する
		float targetHeight = center_.y;

		Vector3 moveDir = { diveHorizDir_.x, 1.0f, diveHorizDir_.z };
		moveDir = Math::Normalize(moveDir);

		trans.translate += moveDir * ascentSpeed_ * Time::GetDeltaTime();

		// 進行方向（前上）を向く
		targetRot.y = std::atan2(moveDir.x, moveDir.z) + 1.570796f;

		// 上昇中は背中を上にした水平姿勢に戻りながら通過する（ひっくり返り防止）
		targetRot.z = 0.0f;
		targetRot.x = 0.0f;

		// 高度が十分に復帰したら、Circleに戻る
		if (trans.translate.y >= targetHeight) {
			state_ = BirdState::Circle;
			stateTimer_ = 0.0f;

			// 通り抜けた後の鳥の位置から実効半径 currentRadius_ と角度 angle_ を正確に計算
			Vector3 offset = trans.translate - center_;
			offset.y = 0.0f; // 水平距離
			currentRadius_ = Math::Length(offset);
			if (currentRadius_ < 1.0f) currentRadius_ = radius_;

			angle_ = std::atan2(offset.z, offset.x);
		}
		break;
	}
	}

	// 角度を最短ルートで滑らかに補間する（360度大回転スピン防止）
	float rotLerpRate = rotLerpSpeed_ * Time::GetDeltaTime();
	if (rotLerpRate > 1.0f) rotLerpRate = 1.0f;
	trans.rotate = LerpEuler(trans.rotate, targetRot, rotLerpRate);
}

void BirdEnemyComponent::ImGui() {
	ImGui::DragFloat3("Center", &center_.x, 0.1f);
	ImGui::DragFloat("Radius", &radius_, 0.1f, 0.0f, 100.0f);
	ImGui::DragFloat("Detection Range (Territory)", &detectionRange_, 0.5f, 0.0f, 200.0f);
	ImGui::DragFloat("Speed", &speed_, 0.05f, -10.0f, 10.0f);
	ImGui::DragInt("HP", &hp_, 1, 0, maxHp_);
	ImGui::DragInt("Max HP", &maxHp_, 1, 1, 50);
	ImGui::Separator();
	ImGui::DragFloat("Circle Duration", &circleDuration_, 0.1f, 0.0f, 20.0f);
	ImGui::DragFloat("Dive Speed", &diveSpeed_, 0.5f, 0.0f, 100.0f);
	ImGui::DragFloat("Ascent Speed", &ascentSpeed_, 0.5f, 0.0f, 100.0f);
	ImGui::DragFloat("Base Height", &baseHeight_, 0.1f, 0.0f, 20.0f);
	ImGui::DragFloat("Rotation Lerp Speed", &rotLerpSpeed_, 0.1f, 0.1f, 50.0f);

	const char* stateStr = "Unknown";
	if (state_ == BirdState::Patrol) stateStr = "Patrol (Waiting)";
	else if (state_ == BirdState::Circle) stateStr = "Circle (Warning)";
	else if (state_ == BirdState::Dive) stateStr = "Dive";
	else if (state_ == BirdState::Ascent) stateStr = "Ascent";
	ImGui::Text("Current State: %s", stateStr);
}

void BirdEnemyComponent::Serialize(json& j) const {
	j["type"] = "BirdEnemyComponent";
	j["center"] = { center_.x, center_.y, center_.z };
	j["radius"] = radius_;
	j["speed"] = speed_;
	j["angle"] = angle_;
	j["hp"] = hp_;
	j["maxHp"] = maxHp_;
	j["circleDuration"] = circleDuration_;
	j["diveSpeed"] = diveSpeed_;
	j["ascentSpeed"] = ascentSpeed_;
	j["baseHeight"] = baseHeight_;
	j["rotLerpSpeed"] = rotLerpSpeed_;
}

void BirdEnemyComponent::Deserialize(const json& j) {
	isInitialized_ = true;
	if (j.contains("center")) {
		center_ = { j["center"][0], j["center"][1], j["center"][2] };
	}
	if (j.contains("radius")) radius_ = j["radius"];
	if (j.contains("speed")) speed_ = j["speed"];
	if (j.contains("angle")) angle_ = j["angle"];
	if (j.contains("hp")) hp_ = j["hp"];
	if (j.contains("maxHp")) maxHp_ = j["maxHp"];
	if (j.contains("circleDuration")) circleDuration_ = j["circleDuration"];
	if (j.contains("diveSpeed")) diveSpeed_ = j["diveSpeed"];
	if (j.contains("ascentSpeed")) ascentSpeed_ = j["ascentSpeed"];
	if (j.contains("baseHeight")) baseHeight_ = j["baseHeight"];
	if (j.contains("rotLerpSpeed")) rotLerpSpeed_ = j["rotLerpSpeed"];
}

void BirdEnemyComponent::TakeDamage(int damage) {
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

void BirdEnemyComponent::OnDead() {
	if (isDead_) return;
	isDead_ = true;
	deathTimer_ = 0.0f;
	if (gameObject_) {
		originalScale_ = gameObject_->GetTransform().scale;
		originalPosition_ = gameObject_->GetTransform().translate;

		// 被弾位置に爆発パーティクルを生成
		ParticleSpawner::SpawnExplosion(gameObject_->GetContext(), gameObject_->GetTransform().translate, 15);

		// GameDirectorへの撃破通知
		if (gameObject_->GetContext() && gameObject_->GetContext()->activeGameObjects) {
			for (auto& obj : *(gameObject_->GetContext()->activeGameObjects)) {
				if (auto* director = obj->GetComponent<GameDirectorComponent>()) {
					director->NotifyEnemyDead();
					break;
				}
			}
		}
	}
}