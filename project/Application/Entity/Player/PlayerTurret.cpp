#include "PCH.h"
#include "PlayerTurret.h"
#include "GameObject.h"
#include "MeshRendererComponent.h"
#include "Model.h"
#include "Skeleton.h"
#include "CameraOrganizer.h"
#include "BaseCamera.h"
#include "ReticleComponent.h"
#include "MathFunction.h"
#include <cmath>

void PlayerTurret::Initialize() {
	initialCanonRotation_ = { 0.0f, 0.0f, 0.0f, 1.0f };
	hasCapturedCanonInitialRot_ = false;
	canonOffsetYaw_ = 0.0f;
	canonOffsetPitch_ = 0.0f;
}

void PlayerTurret::Update(GameObject* gameObject, GameObject* reticleObject) {
	if (!gameObject) return;

	auto* meshRenderer = gameObject->GetComponent<MeshRendererComponent>();
	if (!meshRenderer) return;

	auto* model = meshRenderer->GetModel();
	if (!model) return;

	// "Canon" ノードを取得
	auto* canonNode = model->FindNode("Canon");
	if (!canonNode) return;

	// 初回読み込み時の Canon ノードの初期姿勢をキャプチャ
	if (!hasCapturedCanonInitialRot_) {
		initialCanonRotation_ = canonNode->transform.rotate;
		hasCapturedCanonInitialRot_ = true;
	}

	// 狙っているターゲット位置（カメラ正面のレティクル位置固定）を取得
	CameraData& cameraData = CameraOrganizer::GetInstance()->GetCameraData();
	Vector3 camPos = { cameraData.world.m[3][0], cameraData.world.m[3][1], cameraData.world.m[3][2] };
	Vector3 camForward = { cameraData.world.m[2][0], cameraData.world.m[2][1], cameraData.world.m[2][2] };

	Vector3 targetPos = Math::Add(camPos, Math::Multiply(100.0f, camForward));

	if (reticleObject) {
		if (auto* reticleComp = reticleObject->GetComponent<ReticleComponent>()) {
			if (auto* lockOnEnemy = reticleComp->GetLockOnTarget()) {
				targetPos = lockOnEnemy->GetTransform().translate;
			}
		}
	}

	// 自機位置からの方向ベクトル
	Vector3 canonWorldPos = gameObject->GetTransform().translate;
	Vector3 dir = Math::Subtract(targetPos, canonWorldPos);

	if (Math::Length(dir) > 0.001f) {
		dir = Math::Normalize(dir);

		// ワールドでの目標向き（Yaw, Pitch）を算出
		float worldYaw = std::atan2(dir.x, dir.z);
		float xzLen = std::sqrt(dir.x * dir.x + dir.z * dir.z);
		float worldPitch = std::atan2(-dir.y, xzLen);

		// ワールドでの目標回転クォータニオン
		Quaternion qWorldYaw = Math::MakeRotateAxisAngleQuaternion({ 0.0f, 1.0f, 0.0f }, worldYaw);
		Quaternion qWorldPitch = Math::MakeRotateAxisAngleQuaternion({ 1.0f, 0.0f, 0.0f }, worldPitch);
		Quaternion worldTargetRot = Math::Multiply(qWorldYaw, qWorldPitch);

		// 自機のワールド回転クォータニオン
		Quaternion qPlayerYaw = Math::MakeRotateAxisAngleQuaternion({ 0.0f, 1.0f, 0.0f }, gameObject->GetTransform().rotate.y);
		Quaternion qPlayerPitch = Math::MakeRotateAxisAngleQuaternion({ 1.0f, 0.0f, 0.0f }, gameObject->GetTransform().rotate.x);
		Quaternion playerWorldRot = Math::Multiply(qPlayerYaw, qPlayerPitch);

		// 自機のローカル空間での相対ターゲット回転＝ Inverse(playerWorldRot) * worldTargetRot
		Quaternion localTargetRot = Math::Multiply(Math::Inverse(playerWorldRot), worldTargetRot);

		// オフセット回転（ImGui調整用）を適用
		if (canonOffsetYaw_ != 0.0f || canonOffsetPitch_ != 0.0f) {
			Quaternion qOffY = Math::MakeRotateAxisAngleQuaternion({ 0.0f, 1.0f, 0.0f }, canonOffsetYaw_ * (3.14159265f / 180.0f));
			Quaternion qOffP = Math::MakeRotateAxisAngleQuaternion({ 1.0f, 0.0f, 0.0f }, canonOffsetPitch_ * (3.14159265f / 180.0f));
			localTargetRot = Math::Multiply(localTargetRot, Math::Multiply(qOffY, qOffP));
		}

		// 初期姿勢と合成してCanonノードに適用
		canonNode->transform.rotate = Math::Multiply(initialCanonRotation_, localTargetRot);

		// ノード行列の再計算
		model->UpdateNodeTransforms();
	}
}

void PlayerTurret::ImGui() {
#ifdef USEIMGUI
	ImGui::Text("--- Canon Rotation Adjustment ---");
	ImGui::DragFloat("Canon Offset Yaw (Deg)", &canonOffsetYaw_, 0.5f, -180.0f, 180.0f);
	ImGui::DragFloat("Canon Offset Pitch (Deg)", &canonOffsetPitch_, 0.5f, -180.0f, 180.0f);
#endif
}

void PlayerTurret::Serialize(json& j) const {
	j["canonOffsetYaw"] = canonOffsetYaw_;
	j["canonOffsetPitch"] = canonOffsetPitch_;
}

void PlayerTurret::Deserialize(const json& j) {
	if (j.contains("canonOffsetYaw")) canonOffsetYaw_ = j["canonOffsetYaw"];
	if (j.contains("canonOffsetPitch")) canonOffsetPitch_ = j["canonOffsetPitch"];
}
