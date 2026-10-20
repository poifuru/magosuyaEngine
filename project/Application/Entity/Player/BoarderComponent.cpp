#include "PCH.h"
#include "BoarderComponent.h"
#include "GameObject.h"
#include "BaseScene.h"
#include "Time.h"
#include "MathFunction.h"
#include "InputManager.h"
#include "RawInput.h"
#include "GamePad.h"
#include "WaterSurfaceComponent.h"
#include "BoatWakeComponent.h"
#include <algorithm>
#include <cmath>

void BoarderComponent::Initialize() {
	if (isInitialized_) return;
	state_ = State::Riding;
	hasPrevBoatPos_ = false;
	hasPrevXZ_ = false;
	isInitialized_ = true;
}

void BoarderComponent::Update() {
	if (!gameObject_) return;

	float dt = Time::GetDeltaTime();
	if (dt <= 0.0f) return;

	// 船（BoatWakeComponent を持っているオブジェクト）を探す
	auto* wake = gameObject_->FindComponentInScene<BoatWakeComponent>();
	if (!wake || !wake->GetGameObject()) return;
	const Vector3& boatPos = wake->GetGameObject()->GetTransform().translate;

	// 船の進行方向：移動量から求める（航跡は進んでいる方向にできるため）
	if (hasPrevBoatPos_) {
		Vector2 move = { boatPos.x - prevBoatPos_.x, boatPos.z - prevBoatPos_.z };
		if (Math::Length(move) > 0.001f) {
			boatDir_ = Math::Normalize(move);
		}
	}
	else {
		// 初回は船の向き（yaw）から。PlayerMovement と同じ (sin, cos)
		float yaw = wake->GetGameObject()->GetTransform().rotate.y;
		boatDir_ = { std::sin(yaw), std::cos(yaw) };
	}

	boatVel_ = { (boatPos.x - prevBoatPos_.x) / dt, (boatPos.z - prevBoatPos_.z) / dt };
	boatVel_ = hasPrevBoatPos_ ? boatVel_ : Vector2{ 0.0f, 0.0f };

	prevBoatPos_ = boatPos;
	hasPrevBoatPos_ = true;

	UpdateRope(dt, boatPos, boatDir_);
	UpdateBoard(dt);
}

void BoarderComponent::UpdateRope(float dt, const Vector3& boatPos, const Vector2& boatDir) {
	// 目標の振り出し角
	float targetAngle = ReadSwingInput() * maxAngle_;

	// ばね＋臨界減衰で追いかける（素早く振ると勢いがついて、速く航跡を横切れる）
	float accel = (targetAngle - angle_) * angleStiffness_ * angleStiffness_
		- angleVel_ * 2.0f * angleStiffness_;
	angleVel_ += accel * dt;
	angle_ += angleVel_ * dt;
	angle_ = std::clamp(angle_, -maxAngle_ * 1.2f, maxAngle_ * 1.2f);

	// XZ平面での右方向（forward = (0,1) のとき right = (1,0)）
	Vector2 right = { boatDir.y, -boatDir.x };

	// 船尾（ロープの付け根）から後ろへ、振り出し角だけ横に振った位置
	Vector2 stern = Vector2(boatPos.x, boatPos.z) - boatDir * sternOffset_;
	Vector2 dir = (-boatDir) * std::cos(angle_) + right * std::sin(angle_);
	Vector2 xz = stern + dir * ropeLength_;

	auto& t = gameObject_->GetTransform();
	t.translate.x = xz.x;
	t.translate.z = xz.y;

	// 水平速度（前フレームとの位置の差）
	if (hasPrevXZ_) {
		horizVel_ = (xz - prevXZ_) / dt;
	}
	prevXZ_ = xz;
	hasPrevXZ_ = true;

	// 体は船の進行方向を向く（トリック中の回転はあとで別に足す）
	if (state_ == State::Riding) {
		t.rotate.y = std::atan2(boatDir.x, boatDir.y);
	}
}

void BoarderComponent::UpdateBoard(float dt) {
	auto& t = gameObject_->GetTransform();

	auto* water = gameObject_->FindComponentInScene<WaterSurfaceComponent>();
	auto* wake = gameObject_->FindComponentInScene<BoatWakeComponent>();
	if (!water || !wake) return;

	// 波紋は GPU から1フレーム遅れで届く
	auto& query = wake->GetWaterQuery();
	query.SetQueryPoint(kQuerySlot, t.translate.x, t.translate.z);
	const auto& r = query.GetResult(kQuerySlot);

	// 水面の高さと傾き = Gerstner（CPU） + 波紋（GPU）
	float gdx = 0.0f, gdz = 0.0f;
	water->SampleGerstnerSlope(t.translate.x, t.translate.z, gdx, gdz);
	waterY_ = water->SampleGerstnerHeight(t.translate.x, t.translate.z) + r.height;
	Vector2 grad = { gdx + r.dhdx, gdz + r.dhdz };

	// 足元の水面が持ち上がる速さ：斜面を速く・鋭く横切るほど大きい
	Vector2 relVel = horizVel_ - boatVel_;
	surfaceRise_ = grad.x * relVel.x + grad.y * relVel.y;

	riseMemory_ = std::clamp((std::max)(surfaceRise_, riseMemory_ - riseMemoryDecay_ * dt), 0.0f, 8.0f);

	riseHistory_[historyIndex_] = surfaceRise_;
	historyIndex_ = (historyIndex_ + 1) % kHistorySize;

	bool jumpHeld = IsJumpHeld();
	bool jumpReleased = wasJumpHeld_ && !jumpHeld;
	wasJumpHeld_ = jumpHeld;

	if (state_ == State::Riding) {
		t.translate.y = waterY_;
	
		// 押している間ためて、離した瞬間にジャンプ
		if (jumpReleased) {
			float maxVy = std::sqrt(2.0f * gravity_ * maxJumpHeight_); // 高さ H に必要な初速 √(2gH)
			vy_ = (std::min)((std::max)(riseMemory_, 0.0f) * launchScale_ + baseJump_, maxVy);
			state_ = State::Airborne;
			takeoffWaterY_ = waterY_;
			currentJumpHeight_ = 0.0f;
		}
	}
	else { // Airborne
		vy_ -= gravity_ * dt;
		t.translate.y += vy_ * dt;
		currentJumpHeight_ = (std::max)(currentJumpHeight_, t.translate.y - takeoffWaterY_);

		// 着地
		if (t.translate.y <= waterY_ && vy_ < 0.0f) {
			t.translate.y = waterY_;
			state_ = State::Riding;
			riseMemory_ = 0.0f;
			lastJumpHeight_ = currentJumpHeight_;
			bestJumpHeight_ = (std::max)(bestJumpHeight_, lastJumpHeight_);
			// ステップ3：ここで着水の波紋を注入する
		}
	}
}

float BoarderComponent::ReadSwingInput() const {
	InputManager* input = InputManager::GetInstance();
	float x = 0.0f;

	if (input->GetRawInput()->Push(VK_LEFT))  { x -= 1.0f; }
	if (input->GetRawInput()->Push(VK_RIGHT)) { x += 1.0f; }

	if (input->GetGamePad()->IsConection()) {
		Vector2 stick = input->GetGamePad()->GetStick(useRightStick_ ? LR::Right : LR::Left);
		if (stick.x != 0.0f) { x = stick.x; }
	}
	return std::clamp(x, -1.0f, 1.0f);
}

bool BoarderComponent::IsJumpHeld() const {
	InputManager* input = InputManager::GetInstance();
	if (input->GetRawInput()->Push(VK_UP)) { return true; }
	if (input->GetGamePad()->IsConection() && input->GetGamePad()->PushButton(Button::A)) { return true; }
	return false;
}

void BoarderComponent::ImGui() {
#ifdef USEIMGUI
	ImGui::Text("--- Boarder ---");
	ImGui::Text("state: %s", state_ == State::Riding ? "Riding" : "Airborne");
	ImGui::Text("angle %.2f rad  speed %.2f m/s", angle_, Math::Length(horizVel_));
	ImGui::Text("waterY %.3f  surfaceRise %.2f m/s  memory %.2f", waterY_, surfaceRise_, riseMemory_);
	ImGui::PlotLines("surfaceRise", riseHistory_, kHistorySize, historyIndex_, nullptr, -5.0f, 5.0f, ImVec2(0, 60));
	ImGui::Text("jump: last %.2f m  best %.2f m", lastJumpHeight_, bestJumpHeight_);

	ImGui::Checkbox("Use Right Stick (test)", &useRightStick_);
	ImGui::DragFloat("Rope Length", &ropeLength_, 0.1f, 3.0f, 40.0f);
	ImGui::DragFloat("Stern Offset", &sternOffset_, 0.1f, 0.0f, 10.0f);
	ImGui::DragFloat("Max Angle", &maxAngle_, 0.01f, 0.1f, 1.4f);
	ImGui::DragFloat("Angle Stiffness", &angleStiffness_, 0.1f, 0.5f, 20.0f);
	ImGui::DragFloat("Launch Scale", &launchScale_, 0.1f, 0.0f, 10.0f);
	ImGui::DragFloat("Base Jump", &baseJump_, 0.1f, 0.0f, 10.0f);
	ImGui::DragFloat("Gravity", &gravity_, 0.1f, 1.0f, 30.0f);
	ImGui::DragFloat("Rise Memory Decay", &riseMemoryDecay_, 0.1f, 0.0f, 50.0f);
#endif
}

void BoarderComponent::Serialize(json& j) const {
	j["type"] = "BoarderComponent";
	j["ropeLength"] = ropeLength_;
	j["sternOffset"] = sternOffset_;
	j["maxAngle"] = maxAngle_;
	j["angleStiffness"] = angleStiffness_;
	j["launchScale"] = launchScale_;
	j["baseJump"] = baseJump_;
	j["gravity"] = gravity_;
	j["riseMemoryDecay"] = riseMemoryDecay_;
	j["useRightStick"] = useRightStick_;
}

void BoarderComponent::Deserialize(const json& j) {
	if (j.contains("ropeLength")) ropeLength_ = j["ropeLength"];
	if (j.contains("sternOffset")) sternOffset_ = j["sternOffset"];
	if (j.contains("maxAngle")) maxAngle_ = j["maxAngle"];
	if (j.contains("angleStiffness")) angleStiffness_ = j["angleStiffness"];
	if (j.contains("launchScale")) launchScale_ = j["launchScale"];
	if (j.contains("baseJump")) baseJump_ = j["baseJump"];
	if (j.contains("gravity")) gravity_ = j["gravity"];
	if (j.contains("riseMemoryDecay")) riseMemoryDecay_ = j["riseMemoryDecay"];
	if (j.contains("useRightStick")) useRightStick_ = j["useRightStick"];
}
