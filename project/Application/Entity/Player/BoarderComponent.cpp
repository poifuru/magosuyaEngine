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
	angle_ = 0.0f;
	angleVel_ = 0.0f;
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
	const auto& boatTransform = wake->GetGameObject()->GetTransform();

	UpdateBoat(dt, boatTransform.translate, boatTransform.rotate.y);
	UpdateRope(dt);
	UpdateBoard(dt);
}

void BoarderComponent::UpdateBoat(float dt, const Vector3& boatPos, float boatYaw) {
	if (hasPrevBoatPos_) {
		Vector2 move = { boatPos.x - prevBoatPos_.x, boatPos.z - prevBoatPos_.z };

		// 進行方向は移動量から求める（航跡は進んでいる方向にできるため）
		if (Math::Length(move) > 0.001f) {
			boatDir_ = Math::Normalize(move);
		}
		boatVel_ = move / dt;
	}
	else {
		// 初回は船の向き（yaw）から。PlayerMovement と同じ (sin, cos)
		boatDir_ = { std::sin(boatYaw), std::cos(boatYaw) };
		boatVel_ = { 0.0f, 0.0f };
	}

	boatXZ_ = { boatPos.x, boatPos.z };
	prevBoatPos_ = boatPos;
	hasPrevBoatPos_ = true;
}

void BoarderComponent::UpdateRope(float dt) {
	// 振り出し角に力をかける（水上でも慣性が効くので、直前の入力で向きを変えられない）
	float input = ReadSwingInput();
	bool inAir = (state_ == State::Airborne);

	float accel = input * edgeAccel_ * (inAir ? airControl_ : 1.0f) // エッジで切る（空中はほぼ効かない）
		- angle_ * ropePull_                                          // ロープが真後ろへ引き戻す
		- angleVel_ * (inAir ? 0.0f : waterDrag_);                    // 水の抵抗（空中はなし）

	angleVel_ += accel * dt;
	angle_ += angleVel_ * dt;

	// 限界に当たったら、その方向の勢いは消す（ロープがピンと張る）
	if (angle_ > maxAngle_)  { angle_ = maxAngle_;  if (angleVel_ > 0.0f) angleVel_ = 0.0f; }
	if (angle_ < -maxAngle_) { angle_ = -maxAngle_; if (angleVel_ < 0.0f) angleVel_ = 0.0f; }

	// XZ平面での右方向（forward = (0,1) のとき right = (1,0)）
	Vector2 right = { boatDir_.y, -boatDir_.x };

	// 船尾（ロープの付け根）から後ろへ、振り出し角だけ横に振った位置
	Vector2 stern = boatXZ_ - boatDir_ * sternOffset_;
	Vector2 dir = (-boatDir_) * std::cos(angle_) + right * std::sin(angle_);
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
	if (!inAir) {
		t.rotate.y = std::atan2(boatDir_.x, boatDir_.y);
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

	// 足元の水面が持ち上がる速さ
	// 航跡は船と一緒に動くので、船に対する相対速度で「どれだけ横切っているか」を見る
	Vector2 relVel = horizVel_ - boatVel_;
	surfaceRise_ = grad.x * relVel.x + grad.y * relVel.y;

	riseHistory_[historyIndex_] = surfaceRise_;
	historyIndex_ = (historyIndex_ + 1) % kHistorySize;

	bool jumpHeld = IsJumpHeld();
	bool jumpReleased = wasJumpHeld_ && !jumpHeld;
	wasJumpHeld_ = jumpHeld;

	if (state_ == State::Riding) {
		t.translate.y = waterY_;

		// 直近の最大値を覚えておく（ボタンが少し遅れても許す）
		riseMemory_ = std::clamp((std::max)(surfaceRise_, riseMemory_ - riseMemoryDecay_ * dt), 0.0f, maxRiseMemory_);

		// 押している間ためて、離した瞬間にジャンプ
		if (jumpReleased) {
			float maxVy = std::sqrt(2.0f * gravity_ * maxJumpHeight_); // 高さ H に必要な初速 √(2gH)
			vy_ = (std::min)(riseMemory_ * launchScale_ + baseJump_, maxVy);
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

	if (input->GetRawInput()->Push('A')) { x -= 1.0f; }
	if (input->GetRawInput()->Push('D')) { x += 1.0f; }

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

	// 状態の確認
	if (ImGui::CollapsingHeader("Status", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("state: %s", state_ == State::Riding ? "Riding" : "Airborne");
		ImGui::Text("angle %.2f rad (vel %.2f)  speed %.2f m/s", angle_, angleVel_, Math::Length(horizVel_));
		ImGui::Text("waterY %.3f  surfaceRise %.2f m/s  memory %.2f", waterY_, surfaceRise_, riseMemory_);
		ImGui::PlotLines("surfaceRise", riseHistory_, kHistorySize, historyIndex_, nullptr, -5.0f, 5.0f, ImVec2(0, 60));
		ImGui::Text("jump: last %.2f m  best %.2f m", lastJumpHeight_, bestJumpHeight_);
		if (ImGui::Button("Reset Best")) { bestJumpHeight_ = 0.0f; }
	}

	// ロープと横移動
	if (ImGui::CollapsingHeader("Rope / Swing")) {
		ImGui::DragFloat("Rope Length", &ropeLength_, 0.1f, 3.0f, 40.0f);
		ImGui::DragFloat("Stern Offset", &sternOffset_, 0.1f, 0.0f, 10.0f);
		ImGui::DragFloat("Max Angle", &maxAngle_, 0.01f, 0.1f, 1.5f);
		ImGui::DragFloat("Edge Accel", &edgeAccel_, 0.05f, 0.0f, 20.0f);
		ImGui::DragFloat("Rope Pull", &ropePull_, 0.05f, 0.0f, 20.0f);
		ImGui::DragFloat("Water Drag", &waterDrag_, 0.05f, 0.0f, 10.0f);
		ImGui::DragFloat("Air Control", &airControl_, 0.01f, 0.0f, 1.0f);
		ImGui::Text("hold angle = %.2f rad", ropePull_ > 0.0f ? edgeAccel_ / ropePull_ : 0.0f);
	}

	// ジャンプ
	if (ImGui::CollapsingHeader("Jump")) {
		ImGui::DragFloat("Launch Scale", &launchScale_, 0.1f, 0.0f, 10.0f);
		ImGui::DragFloat("Base Jump", &baseJump_, 0.1f, 0.0f, 10.0f);
		ImGui::DragFloat("Max Jump Height", &maxJumpHeight_, 0.1f, 0.5f, 50.0f);
		ImGui::DragFloat("Gravity", &gravity_, 0.1f, 1.0f, 50.0f);
		ImGui::DragFloat("Rise Memory Decay", &riseMemoryDecay_, 0.1f, 0.0f, 50.0f);
		ImGui::DragFloat("Max Rise Memory", &maxRiseMemory_, 0.1f, 0.0f, 20.0f);
		ImGui::Text("air time at max = %.2f s", gravity_ > 0.0f ? 2.0f * std::sqrt(2.0f * maxJumpHeight_ / gravity_) : 0.0f);
	}

	// 入力
	if (ImGui::CollapsingHeader("Input")) {
		ImGui::Checkbox("Use Right Stick (test)", &useRightStick_);
	}
#endif
}

void BoarderComponent::Serialize(json& j) const {
	j["type"] = "BoarderComponent";

	// ロープ・横移動
	j["ropeLength"] = ropeLength_;
	j["sternOffset"] = sternOffset_;
	j["maxAngle"] = maxAngle_;
	j["edgeAccel"] = edgeAccel_;
	j["ropePull"] = ropePull_;
	j["waterDrag"] = waterDrag_;
	j["airControl"] = airControl_;

	// ジャンプ
	j["launchScale"] = launchScale_;
	j["baseJump"] = baseJump_;
	j["maxJumpHeight"] = maxJumpHeight_;
	j["gravity"] = gravity_;
	j["riseMemoryDecay"] = riseMemoryDecay_;
	j["maxRiseMemory"] = maxRiseMemory_;

	// 入力
	j["useRightStick"] = useRightStick_;
}

void BoarderComponent::Deserialize(const json& j) {
	// ロープ・横移動
	if (j.contains("ropeLength")) ropeLength_ = j["ropeLength"];
	if (j.contains("sternOffset")) sternOffset_ = j["sternOffset"];
	if (j.contains("maxAngle")) maxAngle_ = j["maxAngle"];
	if (j.contains("edgeAccel")) edgeAccel_ = j["edgeAccel"];
	if (j.contains("ropePull")) ropePull_ = j["ropePull"];
	if (j.contains("waterDrag")) waterDrag_ = j["waterDrag"];
	if (j.contains("airControl")) airControl_ = j["airControl"];

	// ジャンプ
	if (j.contains("launchScale")) launchScale_ = j["launchScale"];
	if (j.contains("baseJump")) baseJump_ = j["baseJump"];
	if (j.contains("maxJumpHeight")) maxJumpHeight_ = j["maxJumpHeight"];
	if (j.contains("gravity")) gravity_ = j["gravity"];
	if (j.contains("riseMemoryDecay")) riseMemoryDecay_ = j["riseMemoryDecay"];
	if (j.contains("maxRiseMemory")) maxRiseMemory_ = j["maxRiseMemory"];

	// 入力
	if (j.contains("useRightStick")) useRightStick_ = j["useRightStick"];
}
