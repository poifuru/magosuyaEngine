#include "PCH.h"
#include "BulletComponent.h"
#include "GameObject.h"
#include "MathFunction.h"
#include "BaseScene.h"
#include "../../../../Engine/Editor/ParticleEditor/ParticleSpawner.h"

BulletComponent::~BulletComponent() {
	if (outlineObj_) {
		outlineObj_->Destroy();
	}
}

void BulletComponent::Initialize() {
	if (isInitialized_) return;
	isInitialized_ = true;

	speed_ = 30.0f;
	activeTimer_ = 5.0f;
	hasRecordedStart_ = false;
	isSubmerged_ = false;
}

void BulletComponent::Update() {
	if (!gameObject_) return;

	// 発射初期位置の記録
	if (!hasRecordedStart_) {
		startPosition_ = gameObject_->GetTransform().translate;
		hasRecordedStart_ = true;
	}

	// 追尾ターゲットが現在もシーン内に生存しているか安全確認する！
	if (target_) {
		bool isTargetAlive = false;
		auto* context = gameObject_->GetContext();
		if (context && context->activeGameObjects) {
			for (const auto& obj : *(context->activeGameObjects)) {
				if (obj.get() == target_) {
					isTargetAlive = true;
					break;
				}
			}
		}
		// もしすでにシーンから消滅していたら、ターゲットを安全に外す！
		if (!isTargetAlive) {
			target_ = nullptr;
		}
	}

	// ターゲットが生きていれば、その方向へ徐々に弾の向きを曲げる！
	if (target_ && !target_->IsDead()) {
		Vector3 targetPos = target_->GetTransform().translate;
		Vector3 myPos = gameObject_->GetTransform().translate;
		Vector3 toTarget = Math::Subtract(targetPos, myPos);
		if (Math::Length(toTarget) > 0.001f) {
			toTarget = Math::Normalize(toTarget);
			// 旋回力（毎フレームhomingStrength_%ターゲットの方向へ曲がる）
			direction_ = Math::Lerp(direction_, toTarget, homingStrength_);
			direction_ = Math::Normalize(direction_);
		}
	}

	// 移動処理
	auto& trans = gameObject_->GetTransform();
	trans.translate = Math::Add(trans.translate, Math::Multiply(speed_ * Time::GetDeltaTime(), direction_));

	// 弾の回転も進行方向に向ける
	Vector3 bulletRot = { 0.0f, 0.0f, 0.0f };
	if (Math::Length(direction_) > 0.001f) {
		bulletRot.y = std::atan2(direction_.x, direction_.z);
		float xzLength = std::sqrt(direction_.x * direction_.x + direction_.z * direction_.z);
		bulletRot.x = std::atan2(-direction_.y, xzLength);
	}
	trans.rotate = bulletRot;

	// アウトライン用オブジェクトが存在すれば位置・回転を同期する！
	if (outlineObj_) {
		outlineObj_->GetTransform().translate = trans.translate;
		outlineObj_->GetTransform().rotate = trans.rotate;
	}

	// 弾の飛翔軌跡（加算合成の輝く水色トレイル）を生成。水面の下に潜っても100%透けて見える！
	ParticleSpawner::SpawnBulletGlowTrail(gameObject_->GetContext(), trans.translate, direction_);

	Vector3 currentPos = trans.translate;

	// 1. 初めて水面下（y <= 0.1f）に突入した時だけ着弾水飛沫（Splash）を発生（弾は消さずに水中を直進させる！）
	if (currentPos.y <= 0.1f && !isSubmerged_) {
		isSubmerged_ = true;
		ParticleSpawner::SpawnWaterSplash(gameObject_->GetContext(), currentPos, 14);
	}

	// 2. 有効射程距離の制限チェック（ハープーンが届かずに失速消滅）
	float traveledDist = Math::Length(Math::Subtract(currentPos, startPosition_));
	if (traveledDist >= maxDistance_) {
		ParticleSpawner::SpawnWaterSplash(gameObject_->GetContext(), currentPos, 6);
		if (outlineObj_) outlineObj_->Destroy();
		gameObject_->Destroy();
		return;
	}

	// 3. 寿命チェック
	activeTimer_ -= Time::GetDeltaTime();
	if (activeTimer_ <= 0.0f) {
		if (outlineObj_) outlineObj_->Destroy();
		gameObject_->Destroy(); // 親のオブジェクトを破壊（デスフラグを立てる）
	}
}