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
	// 進行方向は船の向き（yaw）から。BoatDriverComponent は yaw の方向にだけ進むので、移動方向と同じになる
	// （移動量から求めると、スタート地点に戻すなどのワープで逆向きになり、ロープが船の前に回り込んでしまう）
	boatDir_ = { std::sin(boatYaw), std::cos(boatYaw) };

	Vector2 move = { boatPos.x - prevBoatPos_.x, boatPos.z - prevBoatPos_.z };
	bool isTeleport = Math::Length(move) > kTeleportDistance;

	if (hasPrevBoatPos_ && !isTeleport) {
		boatVel_ = move / dt;
	}
	else {
		// 初回・ワープした直後は速度0にする（ワープの距離を速度にすると、水面の判定や波が暴れる）
		boatVel_ = { 0.0f, 0.0f };
		hasPrevXZ_ = false; // ボーダー自身の速度も、ワープで跳ねないようにリセット
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

	// 無効中は押していないことにする
	// jumpReleased にも jumpEnabled_ を掛けるのは、押しっぱなしのまま無効になった瞬間に
	// 「前フレーム押してた・今フレーム離した」= リリース判定になってジャンプが出てしまうのを防ぐため
	bool jumpHeld = jumpEnabled_ && IsJumpHeld();
	bool jumpReleased = jumpEnabled_ && wasJumpHeld_ && !jumpHeld;
	wasJumpHeld_ = jumpHeld;

	if (state_ == State::Riding) {
		t.translate.y = waterY_;

		// 滑っている間：ボードが水を押しのけて、細い航跡を立てる
		if (rideWaveStrength_ > 0.0f || rideFoam_ > 0.0f) {
			WaveSource src{};
			src.position = { t.translate.x, t.translate.z };
			src.velocity = horizVel_;
			src.radius = rideWaveRadius_;
			src.strength = rideWaveStrength_;
			src.foam = rideFoam_;
			wake->AddWaveSource(src);
		}

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
			// 着水：落ちる速さに比例して水面を押し込む（1フレームだけ）→ 円い波紋が広がる
			lastLandingDepth_ = (std::min)(-vy_ * landingDepthScale_, maxLandingDepth_);
			WaveSource splash{};
			splash.position = { t.translate.x, t.translate.z };
			splash.velocity = horizVel_;
			splash.radius = landingRadius_;
			splash.strength = 0.0f; // 動きの波は滑っている方で入れるので、ここは押し込みだけ
			splash.impulse = -lastLandingDepth_;
			splash.foam = landingFoam_;
			wake->AddWaveSource(splash);

			t.translate.y = waterY_;
			state_ = State::Riding;
			riseMemory_ = 0.0f;
			lastJumpHeight_ = currentJumpHeight_;
			bestJumpHeight_ = (std::max)(bestJumpHeight_, lastJumpHeight_);
		}
	}
}

float BoarderComponent::ReadSwingInput() const {
	if (!swingEnabled_) return 0.0f;

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
	if (input->GetRawInput()->Push(VK_SPACE)) { return true; }
	if (input->GetGamePad()->IsConection() && input->GetGamePad()->PushButton(Button::A)) { return true; }
	return false;
}

void BoarderComponent::ImGui() {
#ifdef USEIMGUI
	ImGui::Text("--- プレイヤー ---");

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
	if (ImGui::CollapsingHeader("Water Waves")) {
		ImGui::DragFloat("Ride Wave Strength", &rideWaveStrength_, 0.01f, 0.0f, 3.0f);
		ImGui::DragFloat("Ride Wave Radius", &rideWaveRadius_, 0.05f, 0.3f, 10.0f, "%.2f m");
		ImGui::DragFloat("Ride Foam", &rideFoam_, 0.001f, 0.0f, 0.5f, "%.3f");
		ImGui::DragFloat("Landing Depth Scale", &landingDepthScale_, 0.005f, 0.0f, 1.0f, "%.3f");
		ImGui::DragFloat("Max Landing Depth", &maxLandingDepth_, 0.05f, 0.0f, 5.0f, "%.2f m");
		ImGui::DragFloat("Landing Radius", &landingRadius_, 0.05f, 0.3f, 15.0f, "%.2f m");
		ImGui::DragFloat("Landing Foam", &landingFoam_, 0.01f, 0.0f, 1.0f);
		ImGui::Text("last landing depth: %.2f m", lastLandingDepth_);
	}

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

	j["rideWaveStrength"] = rideWaveStrength_;
	j["rideWaveRadius"] = rideWaveRadius_;
	j["rideFoam"] = rideFoam_;
	j["landingDepthScale"] = landingDepthScale_;
	j["maxLandingDepth"] = maxLandingDepth_;
	j["landingRadius"] = landingRadius_;
	j["landingFoam"] = landingFoam_;

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

	if (j.contains("rideWaveStrength")) rideWaveStrength_ = j["rideWaveStrength"];
	if (j.contains("rideWaveRadius")) rideWaveRadius_ = j["rideWaveRadius"];
	if (j.contains("rideFoam")) rideFoam_ = j["rideFoam"];
	if (j.contains("landingDepthScale")) landingDepthScale_ = j["landingDepthScale"];
	if (j.contains("maxLandingDepth")) maxLandingDepth_ = j["maxLandingDepth"];
	if (j.contains("landingRadius")) landingRadius_ = j["landingRadius"];
	if (j.contains("landingFoam")) landingFoam_ = j["landingFoam"];

	// 入力
	if (j.contains("useRightStick")) useRightStick_ = j["useRightStick"];
}
