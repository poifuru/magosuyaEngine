#include "PCH.h"
#include "BoatDriverComponent.h"
#include "GameObject.h"
#include "Time.h"
#include <algorithm>
#include <cmath>

void BoatDriverComponent::Initialize() {
	if (isInitialized_) return;
	isInitialized_ = true;

	// エディタで置いた位置と向きをスタート地点にする
	if (gameObject_) {
		const auto& t = gameObject_->GetTransform();
		startPos_ = t.translate;
		startYaw_ = t.rotate.y;
	}
	yaw_ = startYaw_;
	speed_ = 0.0f;
	distance_ = 0.0f;
}

void BoatDriverComponent::Update() {
	if (!gameObject_) return;

	float dt = Time::GetDeltaTime();
	if (dt <= 0.0f) return;

	auto& t = gameObject_->GetTransform();

	// 速度を巡航速度に近づける（停止中は同じ加速で減速する）
	float targetSpeed = isRunning_ ? cruiseSpeed_ : 0.0f;
	if (speed_ < targetSpeed) {
		speed_ = std::min(speed_ + acceleration_ * dt, targetSpeed);
	} else {
		speed_ = std::max(speed_ - acceleration_ * dt, targetSpeed);
	}

	// 進行方向へ進める（BoarderComponent と同じく yaw = atan2(x, z) の向き）
	float step = speed_ * dt;
	t.translate.x += std::sin(yaw_) * step;
	t.translate.z += std::cos(yaw_) * step;
	t.translate.y = boatHeight_;
	t.rotate.y = yaw_;

	distance_ += step;
}

void BoatDriverComponent::ResetToStart() {
	if (!gameObject_) return;
	auto& t = gameObject_->GetTransform();
	t.translate = startPos_;
	t.rotate.y = startYaw_;
	yaw_ = startYaw_;
	speed_ = 0.0f;
	distance_ = 0.0f;
}

void BoatDriverComponent::ImGui() {
#ifdef USEIMGUI
	ImGui::Text("--- Boat Driver ---");

	// 状態の確認
	if (ImGui::CollapsingHeader("Status", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("speed %.2f m/s (%.1f km/h)", speed_, speed_ * 3.6f);
		ImGui::Text("yaw %.2f rad  distance %.1f m", yaw_, distance_);
		ImGui::Checkbox("Running", &isRunning_);
		if (ImGui::Button("Reset To Start")) { ResetToStart(); }
		ImGui::SameLine();
		if (ImGui::Button("Set Start Here")) {
			// 今の位置と向きをスタート地点にする（保存はされない）
			startPos_ = gameObject_->GetTransform().translate;
			startYaw_ = gameObject_->GetTransform().rotate.y;
			yaw_ = startYaw_;
		}
	}

	// 走行パラメータ
	if (ImGui::CollapsingHeader("Drive", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragFloat("Cruise Speed [m/s]", &cruiseSpeed_, 0.1f, 0.0f, 40.0f);
		ImGui::DragFloat("Acceleration [m/s2]", &acceleration_, 0.05f, 0.1f, 20.0f);
		ImGui::DragFloat("Boat Height", &boatHeight_, 0.01f, -2.0f, 2.0f);
		if (acceleration_ > 0.0f) {
			ImGui::Text("time to cruise: %.1f s", cruiseSpeed_ / acceleration_);
		}
	}
#endif
}

void BoatDriverComponent::Serialize(json& j) const {
	j["type"] = "BoatDriverComponent";
	j["cruiseSpeed"] = cruiseSpeed_;
	j["acceleration"] = acceleration_;
	j["boatHeight"] = boatHeight_;
	j["isRunning"] = isRunning_;
}

void BoatDriverComponent::Deserialize(const json& j) {
	if (j.contains("cruiseSpeed")) cruiseSpeed_ = j["cruiseSpeed"];
	if (j.contains("acceleration")) acceleration_ = j["acceleration"];
	if (j.contains("boatHeight")) boatHeight_ = j["boatHeight"];
	if (j.contains("isRunning")) isRunning_ = j["isRunning"];
}
