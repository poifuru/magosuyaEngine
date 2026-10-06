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
#include <algorithm>
#include <cmath>

VirtualFollowCamera::~VirtualFollowCamera() {
	if (target_) {
		if (auto* mesh = target_->GetComponent<MeshRendererComponent>()) {
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
		if (auto* mesh = target_->GetComponent<MeshRendererComponent>()) {
			mesh->SetVisible(!isFirstPerson_);
		}
	}
}

void VirtualFollowCamera::Update() {
	if (target_ && gameObject_) {
		InputManager* input = InputManager::GetInstance();

		// Vキーまたはゲームパッドの右スティック押し込みで三人称/一人称の切り替え
		if (input->GetRawInput()->Trigger('V')) {
			SetFirstPerson(!isFirstPerson_);
		}
		if (input->GetGamePad()->IsConection()) {
			if (input->GetGamePad()->TriggerButton(Button::PUSHIN_RIGHT)) {
				SetFirstPerson(!isFirstPerson_);
			}
		}

		// マウス操作によるカメラの角度更新（右クリック不要で常に追従）
		float mouseDeltaX = static_cast<float>(input->GetRawInput()->GetMouseDeltaX());
		float mouseDeltaY = static_cast<float>(input->GetRawInput()->GetMouseDeltaY());
		if (mouseDeltaX != 0.0f || mouseDeltaY != 0.0f) {
			angleY_ += mouseDeltaX * mouseSensitivity_;
			angleX_ += mouseDeltaY * mouseSensitivity_;
		}

		// ゲームパッドの右スティックでカメラの角度を更新する
		if (input->GetGamePad()->IsConection()) {
			Vector2 rStick = input->GetGamePad()->GetStick(LR::Right);
			if (rStick.x != 0.0f || rStick.y != 0.0f) {
				angleY_ += rStick.x * padSensitivity_;
				angleX_ -= rStick.y * padSensitivity_; // 上下ノーマル操作（引いて上、倒して下）
			}
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
			Matrix4x4 rotY = Math::MakeRotateYMatrix(angleY_);
			Matrix4x4 rotMatrix = Math::Multiply(rotX, rotY);

			// 回転させたオフセットをプレイヤーの座標に足して「目標位置」を計算する
			Vector3 rotatedOffset = Math::Transform(offset_, rotMatrix);
			Vector3 targetPos = Math::Add(target_->GetTransform().translate, rotatedOffset);

			// 自身の座標を滑らかに補間して追従させる
			myTransform.translate = Math::Lerp(myTransform.translate, targetPos, 1.0f - delay_);

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
			Vector3 targetCenter = Math::Add(target_->GetTransform().translate, { 0.0f, targetOffsetY_, 0.0f }); // ターゲットの位置（注視点）
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
	if (ImGui::Checkbox("First Person Mode (V Key)", &isFirstPerson_)) {
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
	ImGui::SliderFloat("Delay", &delay_, 0.0f, 1.0f);
	ImGui::DragFloat("Mouse Sensitivity", &mouseSensitivity_, 0.0001f, 0.0001f, 0.05f, "%.4f");
	ImGui::DragFloat("Pad Sensitivity", &padSensitivity_, 0.001f, 0.001f, 0.2f, "%.3f");
}

void VirtualFollowCamera::Serialize(json& j) const {
	VirtualCameraComponent::Serialize(j);
	j["type"] = "VirtualFollowCamera";
	j["targetName"] = targetName_;
	j["offset"] = { offset_.x, offset_.y, offset_.z };
	j["targetOffsetY"] = targetOffsetY_;
	j["delay"] = delay_;
	j["angleX"] = angleX_;
	j["angleY"] = angleY_;
	j["isFirstPerson"] = isFirstPerson_;
	j["firstPersonOffset"] = { firstPersonOffset_.x, firstPersonOffset_.y, firstPersonOffset_.z };
	j["isUnderwater"] = isUnderwater_;
	j["mouseSensitivity"] = mouseSensitivity_;
	j["padSensitivity"] = padSensitivity_;
}

void VirtualFollowCamera::Deserialize(const json& j) {
	VirtualCameraComponent::Deserialize(j);
	if (j.contains("targetName")) targetName_ = j["targetName"];
	if (j.contains("offset")) {
		offset_ = { j["offset"][0], j["offset"][1], j["offset"][2] };
	}
	if (j.contains("targetOffsetY")) targetOffsetY_ = j["targetOffsetY"];
	if (j.contains("delay")) delay_ = j["delay"];
	if (j.contains("angleX")) angleX_ = j["angleX"];
	if (j.contains("angleY")) angleY_ = j["angleY"];
	if (j.contains("isFirstPerson")) {
		isFirstPerson_ = j["isFirstPerson"];
	}
	if (j.contains("firstPersonOffset")) {
		firstPersonOffset_ = { j["firstPersonOffset"][0], j["firstPersonOffset"][1], j["firstPersonOffset"][2] };
	}
	if (j.contains("isUnderwater")) {
		isUnderwater_ = j["isUnderwater"];
	}
	if (j.contains("mouseSensitivity")) {
		mouseSensitivity_ = j["mouseSensitivity"];
	}
	if (j.contains("padSensitivity")) {
		padSensitivity_ = j["padSensitivity"];
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