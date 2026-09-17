#include "PCH.h"
#include "ParticleComponent.h"
#include "GameObject.h"
#include "MeshRendererComponent.h"
#include "CameraOrganizer.h" // 💡 カメラデータ取得用
#include "BaseCamera.h"

void ParticleComponent::Initialize() {
	if (isInitialized_) return;
	isInitialized_ = true;
}

void ParticleComponent::Update() {
	if (!gameObject_) return;

	// 1. 寿命のカウントダウン
	lifeTime_ -= Time::GetDeltaTime();
	if (lifeTime_ <= 0.0f) {
		gameObject_->Destroy(); // 寿命が尽きたら消滅
		return;
	}

	// 2. 移動と重力の適用
	velocity_.y -= gravity_ * Time::GetDeltaTime();
	auto& trans = gameObject_->GetTransform();
	trans.translate = Math::Add(trans.translate, Math::Multiply(Time::GetDeltaTime(), velocity_));

	// 💡 ビルボード設定（常にカメラの方向を向くように回転をコピーする）
	CameraData& cameraData = CameraOrganizer::GetInstance()->GetCameraData();
	trans.rotate = cameraData.transform.rotate;

	// 3. 徐々にスケールを小さくする
	float ratio = lifeTime_ / maxLifeTime_;
	trans.scale = Math::Multiply(ratio, trans.scale);

	// 4. 色のフェード
	if (hasColorFade_) {
		if (auto* mesh = gameObject_->GetComponent<MeshRendererComponent>()) {
			Vector4 currentColor;
			currentColor.x = endColor_.x + (startColor_.x - endColor_.x) * ratio;
			currentColor.y = endColor_.y + (startColor_.y - endColor_.y) * ratio;
			currentColor.z = endColor_.z + (startColor_.z - endColor_.z) * ratio;
			currentColor.w = endColor_.w + (startColor_.w - endColor_.w) * ratio;

			mesh->SetColor(currentColor);
		}
	}
}

