#include "PCH.h"
#include "PlayerMovement.h"
#include "GameObject.h"
#include "InputManager.h"
#include "RawInput.h"
#include "GamePad.h"
#include "MathFunction.h"
#include "CameraOrganizer.h"
#include "BaseCamera.h"
#include "../../../../Engine/Editor/ParticleEditor/ParticleSpawner.h"
#include <algorithm>
#include <cmath>

void PlayerMovement::Initialize() {
	speed_ = 0.5f;
	velocity_ = { 0.0f, 0.0f, 0.0f };
	turnSpeed_ = 0.3f;
	dirRatioZ_ = 0.97f;
	dirRatioX_ = 0.03f;
	attenuationRate_ = 0.98f;
	brakeAttenuationRate_ = 0.90f;
	maxSpeed_ = 2.5f;
	brakeTurnSpeedMultiplier_ = 3.0f;
	brakeMoveSpeedMultiplier_ = 0.2f;

	isUnderwater_ = false;
	isDiving_ = false;
	targetDepthY_ = -30.0f;
	diveSpeed_ = 15.0f;
}

void PlayerMovement::TransitionToUnderwater() {
	if (isUnderwater_) return;
	isUnderwater_ = true;
	isDiving_ = true;
}

void PlayerMovement::Update(GameObject* gameObject) {
	InputManager::GetInstance()->GetGamePad()->SetStickDeadZone(2000);
	Move(gameObject);
}

void PlayerMovement::Move(GameObject* gameObject) {
	if (!gameObject) return;

	// 潜航演出中の処理
	if (isDiving_) {
		float currentY = gameObject->GetTransform().translate.y;
		if (currentY > targetDepthY_) {
			currentY -= diveSpeed_ * Time::GetDeltaTime();
			if (currentY <= targetDepthY_) {
				currentY = targetDepthY_;
				isDiving_ = false;
			}
		} else {
			isDiving_ = false;
		}
		gameObject->GetTransform().translate.y = currentY;

		// 潜航時に水飛沫エフェクトを発生
		static float splashInterval = 0.0f;
		splashInterval += Time::GetDeltaTime();
		if (splashInterval >= 0.1f && currentY > -5.0f) {
			splashInterval = 0.0f;
			if (gameObject->GetContext()) {
				ParticleSpawner::SpawnWaterSplash(gameObject->GetContext(), gameObject->GetTransform().translate, 5);
			}
		}
	}

	// 起動時に合計が 1.0f からズレていた場合のための自動補正
	dirRatioZ_ = std::clamp(dirRatioZ_, 0.0f, 1.0f);
	dirRatioX_ = 1.0f - dirRatioZ_;

	// 入力用のポインタを取得
	InputManager* input = InputManager::GetInstance();

	// ブレーキ入力の判定
	bool isBrakePressed = false;
	if (!isUnderwater_ && input->GetRawInput()->Push(VK_SPACE)) {
		isBrakePressed = true;
	}
	if (input->GetGamePad()->IsConection()) {
		if (input->GetGamePad()->PushButton(Button::A)) {
			isBrakePressed = true;
		}
	}
	// ブレーキ状態に応じて旋回力と移動力を変動させる
	float currentTurnSpeed = turnSpeed_;
	float currentSpeed = speed_;
	if (isBrakePressed) {
		currentTurnSpeed = turnSpeed_ * brakeTurnSpeedMultiplier_;
		currentSpeed = speed_ * brakeMoveSpeedMultiplier_;
	}

	acceleration_ = { 0.0f, 0.0f, 0.0f };
	Vector3 moveDir = { 0.0f, 0.0f, 0.0f };

	// カメラのワールド行列から方向ベクトルを取得
	Matrix4x4 camWorld = CameraOrganizer::GetInstance()->GetCameraData().world;
	Vector3 camForward = { camWorld.m[2][0], camWorld.m[2][1], camWorld.m[2][2] };
	Vector3 camRight = { camWorld.m[0][0], camWorld.m[0][1], camWorld.m[0][2] };

	// 前後左右の移動は水平方向（XZ平面）に制限して基準ベクトルを作成
	camForward.y = 0.0f;
	camRight.y = 0.0f;
	if (Math::Length(camForward) > 0.001f) camForward = Math::Normalize(camForward);
	if (Math::Length(camRight) > 0.001f) camRight = Math::Normalize(camRight);

	// キーボード入力で移動方向を蓄積
	if (input->GetRawInput()->Push('W')) { moveDir = Math::Add(moveDir, camForward); }
	if (input->GetRawInput()->Push('S')) { moveDir = Math::Subtract(moveDir, camForward); }
	if (input->GetRawInput()->Push('A')) { moveDir = Math::Subtract(moveDir, camRight); }
	if (input->GetRawInput()->Push('D')) { moveDir = Math::Add(moveDir, camRight); }

	// 水中での上下移動（Spaceで上昇、Shiftで下降）
	if (isUnderwater_ && !isDiving_) {
		float vertMove = 0.0f;
		if (input->GetRawInput()->Push(VK_SPACE)) { vertMove += 1.0f; }
		if (input->GetRawInput()->Push(VK_SHIFT)) { vertMove -= 1.0f; }

		if (input->GetGamePad()->IsConection()) {
			if (input->GetGamePad()->PushButton(Button::R)) { vertMove += 1.0f; }
			if (input->GetGamePad()->PushButton(Button::L)) { vertMove -= 1.0f; }
		}

		if (vertMove != 0.0f) {
			velocity_.y += vertMove * currentSpeed * Time::GetDeltaTime() * 2.0f;
		}
	}

	// ゲームパッド入力で移動方向を蓄積
	if (input->GetGamePad()->IsConection()) {
		Vector2 lStick = input->GetGamePad()->GetStick(LR::Left);
		if (lStick.x != 0.0f || lStick.y != 0.0f) {
			moveDir = Math::Add(moveDir, Math::Multiply(lStick.y, camForward));
			moveDir = Math::Add(moveDir, Math::Multiply(lStick.x, camRight));
		}
	}

	// 入力があった場合に移動と回転を設定する
	if (Math::Length(moveDir) > 0.0f) {
		moveDir = Math::Normalize(moveDir);

		// 向き（Yaw回転 / Pitch回転）を徐々に補間して近づける
		float targetYaw = std::atan2(moveDir.x, moveDir.z);
		float currentYaw = gameObject->GetTransform().rotate.y;

		// 角度の最短差分を求める
		float diffYaw = targetYaw - currentYaw;
		while (diffYaw < -3.14159265f) diffYaw += 6.2831853f;
		while (diffYaw > 3.14159265f) diffYaw -= 6.2831853f;

		// 旋回速度 (値が小さいほどゆっくり曲がる)
		gameObject->GetTransform().rotate.y += diffYaw * currentTurnSpeed * Time::GetDeltaTime();

		// 上下の旋回目標角度 (Pitch)
		float xzLength = std::sqrt(moveDir.x * moveDir.x + moveDir.z * moveDir.z);
		float targetPitch = std::atan2(-moveDir.y, xzLength);
		float currentPitch = gameObject->GetTransform().rotate.x;
		float diffPitch = targetPitch - currentPitch;
		while (diffPitch < -3.14159265f) diffPitch += 6.2831853f;
		while (diffPitch > 3.14159265f) diffPitch -= 6.2831853f;
		gameObject->GetTransform().rotate.x += diffPitch * currentTurnSpeed * Time::GetDeltaTime();

		// 移動ベクトルのブレンド (前進 dirRatioZ_ : 入力 dirRatioX_)
		float cy = std::cos(gameObject->GetTransform().rotate.y);
		float sy = std::sin(gameObject->GetTransform().rotate.y);
		float cx = std::cos(gameObject->GetTransform().rotate.x);
		float sx = std::sin(gameObject->GetTransform().rotate.x);

		forward_ = { sy * cx, -sx, cy * cx };
		if (Math::Length(forward_) > 0.001f) {
			forward_ = Math::Normalize(forward_);
		}

		// 実際の進む方向 = (正面方向 * dirRatioZ_) + (入力された移動方向 * dirRatioX_)
		Vector3 actualMoveDir = {};
		actualMoveDir.x = forward_.x * dirRatioZ_ + moveDir.x * dirRatioX_;
		actualMoveDir.y = forward_.y * dirRatioZ_ + moveDir.y * dirRatioX_;
		actualMoveDir.z = forward_.z * dirRatioZ_ + moveDir.z * dirRatioX_;

		if (Math::Length(actualMoveDir) > 0.001f) {
			actualMoveDir = Math::Normalize(actualMoveDir);
		}

		// 加速度を設定
		acceleration_.x = actualMoveDir.x * currentSpeed * Time::GetDeltaTime();
		acceleration_.y = actualMoveDir.y * currentSpeed * Time::GetDeltaTime();
		acceleration_.z = actualMoveDir.z * currentSpeed * Time::GetDeltaTime();
		velocity_.x += acceleration_.x;
		velocity_.y += acceleration_.y;
		velocity_.z += acceleration_.z;
	}

	// 速度の減衰と制限
	float currentAttenuation = attenuationRate_;
	if (isBrakePressed) {
		currentAttenuation = brakeAttenuationRate_;
	}

	velocity_.x *= currentAttenuation;
	velocity_.y *= currentAttenuation;
	velocity_.z *= currentAttenuation;

	// クランプで最高・最低速度を制限
	velocity_.x = std::clamp(velocity_.x, -maxSpeed_, maxSpeed_);
	velocity_.y = std::clamp(velocity_.y, -maxSpeed_, maxSpeed_);
	velocity_.z = std::clamp(velocity_.z, -maxSpeed_, maxSpeed_);

	const float minSpeed = 0.00001f;
	if (std::abs(velocity_.x) < minSpeed) { velocity_.x = 0.0f; }
	if (std::abs(velocity_.y) < minSpeed) { velocity_.y = 0.0f; }
	if (std::abs(velocity_.z) < minSpeed) { velocity_.z = 0.0f; }

	// 位置の加算
	gameObject->GetTransform().translate.x += velocity_.x;
	gameObject->GetTransform().translate.y += velocity_.y;
	gameObject->GetTransform().translate.z += velocity_.z;

	// 水深・水面の制限
	if (isUnderwater_) {
		const float kWaterMaxY = -2.0f;  // 水面より少し下
		const float kWaterMinY = -90.0f; // 海底の手前
		if (gameObject->GetTransform().translate.y > kWaterMaxY) {
			gameObject->GetTransform().translate.y = kWaterMaxY;
			if (velocity_.y > 0.0f) velocity_.y = 0.0f;
		}
		if (gameObject->GetTransform().translate.y < kWaterMinY) {
			gameObject->GetTransform().translate.y = kWaterMinY;
			if (velocity_.y < 0.0f) velocity_.y = 0.0f;
		}
	}
	else {
		// 水上フェーズでは水面に位置を維持
		gameObject->GetTransform().translate.y = 0.3f;
		velocity_.y = 0.0f;
	}
}

void PlayerMovement::ImGui() {
#ifdef USEIMGUI
	ImGui::Separator();
	ImGui::Text("--- Underwater Phase ---");
	if (ImGui::Checkbox("Is Underwater", &isUnderwater_)) {
		if (isUnderwater_) {
			isDiving_ = true;
		}
	}
	ImGui::DragFloat("Target Depth Y", &targetDepthY_, 0.5f, -90.0f, -5.0f);
	ImGui::DragFloat("Dive Speed", &diveSpeed_, 0.5f, 1.0f, 50.0f);

	ImGui::Separator();
	ImGui::DragFloat("Speed (Power)", &speed_, 0.01f, 0.0f, 5.0f);
	ImGui::DragFloat("Max Speed", &maxSpeed_, 0.05f, 0.5f, 10.0f); 
	ImGui::DragFloat("Attenuation (Inertia)", &attenuationRate_, 0.001f, 0.90f, 0.999f);
	ImGui::DragFloat("Brake (Attenuation)", &brakeAttenuationRate_, 0.001f, 0.50f, 0.99f);
	ImGui::DragFloat3("Velocity", &velocity_.x, 0.05f);
	ImGui::DragFloat("turnSpeed", &turnSpeed_, 0.001f);

	if (ImGui::DragFloat("dirRatioZ", &dirRatioZ_, 0.001f, 0.0f, 1.0f)) {
		dirRatioZ_ = std::clamp(dirRatioZ_, 0.0f, 1.0f);
		dirRatioX_ = 1.0f - dirRatioZ_;
	}
	if (ImGui::DragFloat("dirRatioX", &dirRatioX_, 0.001f, 0.0f, 1.0f)) {
		dirRatioX_ = std::clamp(dirRatioX_, 0.0f, 1.0f);
		dirRatioZ_ = 1.0f - dirRatioX_;
	}

	ImGui::Text("--- Brake Behavior Trade-off ---");
	ImGui::SliderFloat("Brake Turn Mult", &brakeTurnSpeedMultiplier_, 1.0f, 10.0f);
	ImGui::SliderFloat("Brake Move Mult", &brakeMoveSpeedMultiplier_, 0.0f, 1.0f);
#endif
}

void PlayerMovement::Serialize(json& j) const {
	j["speed"] = speed_;
	j["maxSpeed"] = maxSpeed_;
	j["attenuation"] = attenuationRate_;
	j["brakeAttenuation"] = brakeAttenuationRate_;
	j["turnSpeed"] = turnSpeed_;
	j["dirRatioZ"] = dirRatioZ_;
	j["dirRatioX"] = dirRatioX_;
	j["brakeTurnSpeedMultiplier"] = brakeTurnSpeedMultiplier_;
	j["brakeMoveSpeedMultiplier"] = brakeMoveSpeedMultiplier_;
	j["isUnderwater"] = isUnderwater_;
	j["targetDepthY"] = targetDepthY_;
}

void PlayerMovement::Deserialize(const json& j) {
	if (j.contains("speed")) speed_ = j["speed"];
	if (j.contains("maxSpeed")) maxSpeed_ = j["maxSpeed"];
	if (j.contains("attenuation")) attenuationRate_ = j["attenuation"];
	if (j.contains("brakeAttenuation")) brakeAttenuationRate_ = j["brakeAttenuation"];
	if (j.contains("turnSpeed")) turnSpeed_ = j["turnSpeed"];
	if (j.contains("dirRatioZ")) dirRatioZ_ = j["dirRatioZ"];
	if (j.contains("dirRatioX")) dirRatioX_ = j["dirRatioX"];
	if (j.contains("brakeTurnSpeedMultiplier")) brakeTurnSpeedMultiplier_ = j["brakeTurnSpeedMultiplier"];
	if (j.contains("brakeMoveSpeedMultiplier")) brakeMoveSpeedMultiplier_ = j["brakeMoveSpeedMultiplier"];
	if (j.contains("isUnderwater")) isUnderwater_ = j["isUnderwater"];
	if (j.contains("targetDepthY")) targetDepthY_ = j["targetDepthY"];
}
