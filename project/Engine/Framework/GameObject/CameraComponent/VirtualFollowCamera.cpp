#include "PCH.h"
#include "VirtualFollowCamera.h"
#include "GameObject.h"
#include "MathFunction.h"
#include "BaseScene.h" // SceneContextやGameObject検索用
#include "InputManager.h"
#include "RawInput.h"
#include "GamePad.h"
#include "MeshRendererComponent.h"
#include "PlayerComponent.h"
#include "BoarderComponent.h"
#include "BoatDriverComponent.h"
#include "Time.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace {
	// 半減期から、このフレームで寄る割合を出す（FPS に左右されない指数補間）
	float HalfLifeT(float halfLife, float dt) {
		if (halfLife <= 0.0f) return 1.0f;
		return 1.0f - std::exp(-0.69314718f * dt / halfLife); // ln2 = 0.693
	}

	// 角度の差を -π～π にする
	float WrapAngle(float a) {
		const float pi = std::numbers::pi_v<float>;
		while (a > pi) a -= 2.0f * pi;
		while (a < -pi) a += 2.0f * pi;
		return a;
	}

	// Unity の SmoothDamp と同じ、オーバーシュートしないバネ（臨界減衰）
	// smoothTime：だいたい目標に追いつくまでの時間[s]
	float SmoothDamp(float current, float target, float& velocity, float smoothTime, float dt) {
		smoothTime = (std::max)(0.0001f, smoothTime);
		float omega = 2.0f / smoothTime;
		float x = omega * dt;
		float e = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x); // exp(-x) の近似
		float change = current - target;
		float temp = (velocity + omega * change) * dt;
		velocity = (velocity - omega * temp) * e;
		return target + (change + temp) * e;
	}
}

VirtualFollowCamera::~VirtualFollowCamera() {
	if (target_) {
		if (auto* mesh = target_->GetComponent<MyEngine::Rendering::MeshRendererComponent>()) {
			mesh->SetVisible(true);
		}
	}
}

void VirtualFollowCamera::SetFirstPerson(bool flag) {
	isFirstPerson_ = flag;
	UpdateTargetVisibility();
}

void VirtualFollowCamera::UpdateTargetVisibility() {
	if (target_) {
		if (auto* mesh = target_->GetComponent<MyEngine::Rendering::MeshRendererComponent>()) {
			mesh->SetVisible(!isFirstPerson_);
		}
	}
}

void VirtualFollowCamera::Update() {
	if (target_ && gameObject_) {
		float dt = Time::GetDeltaTime();

		// ボートの進行方向に合わせる：angleY_ は「進行方向からのずれ」として足す
		float yaw = angleY_;
		auto* boat = followBoatHeading_ ? gameObject_->FindComponentInScene<BoatDriverComponent>() : nullptr;
		if (boat) {
			float heading = boat->GetYaw();

			// ボート → プレイヤーの線の延長上にカメラを置く（カメラはプレイヤー越しにボートを見る）
			if (alignToRope_ && boat->GetGameObject()) {
				const Vector3& boatPos = boat->GetGameObject()->GetTransform().translate;
				const Vector3& playerPos = target_->GetTransform().translate;
				float dx = boatPos.x - playerPos.x;
				float dz = boatPos.z - playerPos.z;
				if (dx * dx + dz * dz > minRopeLength_ * minRopeLength_) {
					heading = std::atan2(dx, dz); // プレイヤーからボートへ向く方向
				}
			}

			if (!hasHeading_) {
				smoothedHeading_ = heading;
				hasHeading_ = true;
			}
			// 角度は -π～π の境目でジャンプするので、差を折り返してから補間する
			smoothedHeading_ += WrapAngle(heading - smoothedHeading_) * HalfLifeT(headingHalfLife_, dt);
			smoothedHeading_ = WrapAngle(smoothedHeading_);
			yaw = smoothedHeading_ + angleY_;
		}

		// 上下（Pitch）の回転角度を制限する（床抜けや頭頂部フリップを防ぐ。約-80度〜80度）
		const float maxPitch = 80.0f * (3.14159265f / 180.0f);
		angleX_ = std::clamp(angleX_, -maxPitch, maxPitch);

		// プレイヤーが水中に入っているかを自動同期
		if (auto* playerComp = target_->GetComponent<PlayerComponent>()) {
			isUnderwater_ = playerComp->IsUnderwater();
		}

		auto& myTransform = gameObject_->GetTransform();

		if (isFirstPerson_) {
			// 一人称視点: 自機位置 + 一人称オフセットに追従（遅延なし）
			Vector3 fpPos = Math::Add(target_->GetTransform().translate, firstPersonOffset_);
			myTransform.translate = fpPos;

			// 視線方向（Yaw, Pitch）を直接カメラの回転として適用
			myTransform.rotate.x = angleX_;
			myTransform.rotate.y = angleY_;
			myTransform.rotate.z = 0.0f;
		}
		else {
			// 三人称視点: 回転行列を作成して、オフセットを回転させる
			Matrix4x4 rotX = Math::MakeRotateXMatrix(angleX_);
			Matrix4x4 rotY = Math::MakeRotateYMatrix(yaw);
			Matrix4x4 rotMatrix = Math::Multiply(rotX, rotY);

			// --- ターゲット位置をなめらかにする（水平はほぼぴったり、高さはゆっくり） ---
			const Vector3& raw = target_->GetTransform().translate;
			if (!hasSmoothedTarget_) {
				smoothedTarget_ = raw;
				smoothedYVel_ = 0.0f;
				hasSmoothedTarget_ = true;
			}
			float tXZ = HalfLifeT(xzHalfLife_, dt);
			smoothedTarget_.x += (raw.x - smoothedTarget_.x) * tXZ;
			smoothedTarget_.z += (raw.z - smoothedTarget_.z) * tXZ;

			// ジャンプ中は高さの遅れを短くする
			bool inAir = false;
			if (auto* boarder = target_->GetComponent<BoarderComponent>()) {
				inAir = boarder->GetState() == BoarderComponent::State::Airborne;
			}
			float yTime = inAir ? airYSmoothTime_ : ySmoothTime_;
			if (dt > 0.0f) {
				smoothedTarget_.y = SmoothDamp(smoothedTarget_.y, raw.y, smoothedYVel_, yTime, dt);
			}

			// 回転させたオフセットを、なめらかにしたターゲット位置に足してカメラの位置にする
			Vector3 rotatedOffset = Math::Transform(offset_, rotMatrix);
			myTransform.translate = Math::Add(smoothedTarget_, rotatedOffset);

			// 水上フェーズでは水面（Y = 0.5f）より下に行かないように制限
			// 水中フェーズでは水面（Y = -0.8f）より上に出ないように制限
			if (!isUnderwater_) {
				const float kMinCameraY = 0.5f; // 水面の高さ
				if (myTransform.translate.y < kMinCameraY) {
					myTransform.translate.y = kMinCameraY;
				}
			} else {
				const float kMaxCameraY = -0.8f; // 水面下
				if (myTransform.translate.y > kMaxCameraY) {
					myTransform.translate.y = kMaxCameraY;
				}
			}

			// 常にターゲットの方を向く（LookAt回転）の計算
			Vector3 targetCenter = Math::Add(smoothedTarget_, { 0.0f, targetOffsetY_, 0.0f }); // 注視点（なめらかにした位置）
			Vector3 direction = Math::Subtract(targetCenter, myTransform.translate); // カメラから見たターゲットへの方向

			if (Math::Length(direction) > 0.001f) {
				// ヨー回転（Y軸まわり）の計算
				myTransform.rotate.y = std::atan2(direction.x, direction.z);
				// ピッチ回転（X軸まわり）の計算
				float xzLength = std::sqrt(direction.x * direction.x + direction.z * direction.z);
				myTransform.rotate.x = std::atan2(-direction.y, xzLength);
				// ロール回転（Z軸まわり）は0にリセット
				myTransform.rotate.z = 0.0f;
			}
		}
	}
}

void VirtualFollowCamera::ImGui() {
	VirtualCameraComponent::ImGui(); // 親のImGui（優先度・FOV）を描画

	ImGui::Separator();
	ImGui::Checkbox("Is Underwater", &isUnderwater_);
	if (ImGui::Checkbox("First Person Mode", &isFirstPerson_)) {
		UpdateTargetVisibility();
	}
	ImGui::DragFloat3("First Person Offset", &firstPersonOffset_.x, 0.1f);

	char nameBuf[256];
	strcpy_s(nameBuf, targetName_.c_str());
	if (ImGui::InputText("Target Object Name", nameBuf, sizeof(nameBuf))) {
		targetName_ = nameBuf;
		target_ = nullptr; // ターゲットを再検索させる
	}
	ImGui::DragFloat3("Offset", &offset_.x, 0.1f);
	ImGui::DragFloat("Target Center Y Offset", &targetOffsetY_, 0.1f, 0.0f, 15.0f);
	ImGui::SeparatorText("Smooth Follow");
	ImGui::DragFloat("XZ Half-life", &xzHalfLife_, 0.005f, 0.0f, 2.0f, "%.3f s");
	ImGui::DragFloat("Y Smooth Time", &ySmoothTime_, 0.01f, 0.0f, 3.0f, "%.2f s");
	ImGui::DragFloat("Air Y Smooth Time", &airYSmoothTime_, 0.01f, 0.0f, 3.0f, "%.2f s");
	ImGui::SeparatorText("Boat Heading");
	ImGui::Checkbox("Follow Boat Heading", &followBoatHeading_);
	ImGui::Checkbox("Align To Rope (Boat - Player line)", &alignToRope_);
	ImGui::DragFloat("Heading Half-life", &headingHalfLife_, 0.01f, 0.0f, 3.0f, "%.2f s");
	ImGui::SeparatorText("Angle");
	ImGui::SliderAngle("Pitch", &angleX_, -80.0f, 80.0f);
	ImGui::SliderAngle("Yaw Offset", &angleY_, -180.0f, 180.0f);
}

void VirtualFollowCamera::Serialize(json& j) const {
	VirtualCameraComponent::Serialize(j);
	j["type"] = "VirtualFollowCamera";
	j["targetName"] = targetName_;
	j["offset"] = { offset_.x, offset_.y, offset_.z };
	j["targetOffsetY"] = targetOffsetY_;
	j["xzHalfLife"] = xzHalfLife_;
	j["ySmoothTime"] = ySmoothTime_;
	j["airYSmoothTime"] = airYSmoothTime_;
	j["followBoatHeading"] = followBoatHeading_;
	j["alignToRope"] = alignToRope_;
	j["headingHalfLife"] = headingHalfLife_;
	j["angleX"] = angleX_;
	j["angleY"] = angleY_;
	j["isFirstPerson"] = isFirstPerson_;
	j["firstPersonOffset"] = { firstPersonOffset_.x, firstPersonOffset_.y, firstPersonOffset_.z };
	j["isUnderwater"] = isUnderwater_;
}

void VirtualFollowCamera::Deserialize(const json& j) {
	VirtualCameraComponent::Deserialize(j);
	if (j.contains("targetName")) targetName_ = j["targetName"];
	if (j.contains("offset")) {
		offset_ = { j["offset"][0], j["offset"][1], j["offset"][2] };
	}
	if (j.contains("targetOffsetY")) targetOffsetY_ = j["targetOffsetY"];
	if (j.contains("xzHalfLife")) xzHalfLife_ = j["xzHalfLife"];
	if (j.contains("ySmoothTime")) ySmoothTime_ = j["ySmoothTime"];
	if (j.contains("airYSmoothTime")) airYSmoothTime_ = j["airYSmoothTime"];
	if (j.contains("followBoatHeading")) followBoatHeading_ = j["followBoatHeading"];
	if (j.contains("alignToRope")) alignToRope_ = j["alignToRope"];
	if (j.contains("headingHalfLife")) headingHalfLife_ = j["headingHalfLife"];
	if (j.contains("angleX")) angleX_ = j["angleX"];
	if (j.contains("angleY")) angleY_ = WrapAngle(j["angleY"].get<float>());
	if (j.contains("isFirstPerson")) {
		isFirstPerson_ = j["isFirstPerson"];
	}
	if (j.contains("firstPersonOffset")) {
		firstPersonOffset_ = { j["firstPersonOffset"][0], j["firstPersonOffset"][1], j["firstPersonOffset"][2] };
	}
	if (j.contains("isUnderwater")) {
		isUnderwater_ = j["isUnderwater"];
	}
	UpdateTargetVisibility();
}

void VirtualFollowCamera::ResolveTarget(const std::vector<std::unique_ptr<GameObject>>& gameObjects) {
	if (targetName_.empty()) return;
	for (const auto& obj : gameObjects) {
		if (obj->GetName() == targetName_) {
			target_ = obj.get();
			UpdateTargetVisibility();
			return;
		}
	}
}