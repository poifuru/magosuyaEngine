#include "PCH.h"
#include "RoughSeaComponent.h"
#include "BoatDriverComponent.h"
#include "WaterSurfaceComponent.h"
#include "GameFlowComponent.h"
#include "DolphinEventComponent.h"
#include "GameObject.h"
#include "Time.h"

#include <algorithm>
#include <cmath>

namespace {
	// 0〜1 をなめらかに（始めと終わりがゆっくり）
	float SmoothStep01(float t) {
		t = std::clamp(t, 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	const char* kStateNames[] = { "待機", "予告", "大きくなる", "いちばん荒れている", "静まる", "終了" };
}

bool RoughSeaComponent::FindReferences() {
	if (!boat_) { boat_ = gameObject_->FindComponentInScene<BoatDriverComponent>(); }
	if (!water_) { water_ = gameObject_->FindComponentInScene<WaterSurfaceComponent>(); }
	if (!flow_) { flow_ = gameObject_->FindComponentInScene<GameFlowComponent>(); }
	if (!dolphin_) { dolphin_ = gameObject_->FindComponentInScene<DolphinEventComponent>(); }
	// GameFlow とイルカは無くても動く（GameFlow が無ければ常に Playing 扱い）
	return boat_ && water_;
}

void RoughSeaComponent::ChangeState(State next) {
	if (next == State::Calming) { calmFrom_ = weight_; }
	state_ = next;
	stateTime_ = 0.0f;
}

void RoughSeaComponent::ApplyWeight(float weight) {
	weight_ = std::clamp(weight, 0.0f, 1.0f);
	if (water_) { water_->SetStormWeight(weight_); }
}

void RoughSeaComponent::StartEvent(float strength) {
	strength_ = std::clamp(strength, 0.0f, 1.0f);

	// うねりの向き = 今の船の向き（yaw = atan2(x, z)）
	// 向きは強さ 0 のときしか変えられないので、念のため 0 にしてから決める
	ApplyWeight(0.0f);
	if (boat_ && water_) {
		float yaw = boat_->GetYaw();
		water_->SetStormDirection({ std::sin(yaw), std::cos(yaw) });
	}
	ChangeState(State::Warning);
}

float RoughSeaComponent::CalcTurnAfter(int index) const {
	if (!boat_ || index < 0) return 0.0f;
	const size_t count = boat_->GetWaypoints().size();
	auto yawOf = [&](size_t i) {
		Vector3 d = boat_->GetIncomingDir(i);
		return std::atan2(d.x, d.z);
	};
	auto wrap = [](float a) { return std::remainder(a, 6.2831853f); }; // -π〜π
	size_t k = static_cast<size_t>(index);
	float turn = 0.0f;
	if (k + 1 < count) { turn += std::fabs(wrap(yawOf(k + 1) - yawOf(k))); }
	if (k + 2 < count) { turn += std::fabs(wrap(yawOf(k + 2) - yawOf(k + 1))); }
	return turn;
}

void RoughSeaComponent::Update() {
	if (!gameObject_) return;
	if (!FindReferences()) return;

	float dt = Time::GetDeltaTime();
	if (dt <= 0.0f) return;

	using Phase = GameFlowComponent::Phase;
	bool isPlaying = !flow_ || flow_->GetPhase() == Phase::Playing;

	// ゴール・ジャンプ終了したら、荒れている途中でも静めていく
	if (!isPlaying && IsBusy()) {
		ChangeState(State::Calming);
	}

	switch (state_) {
	case State::Waiting: {
		if (!isPlaying) break;

		// 印の点を通過した（向かう点がその点から次へ変わった）
		int rough = boat_->GetRoughWaypoint();
		size_t target = boat_->GetTargetIndex();
		if (rough >= 0 && hasPrevTarget_ && prevTarget_ == static_cast<size_t>(rough) && target != prevTarget_) {
			pendingStart_ = true;
			pendingStrength_ = boat_->GetWaypoints()[rough].rough;
		}
		prevTarget_ = target;
		hasPrevTarget_ = true;

		// イルカが並走中なら、終わるまで待つ（抽選で離してあるので、ふつうは待たない。カーブで遅れたときの保険）
		if (pendingStart_ && !(dolphin_ && dolphin_->IsBusy())) {
			pendingStart_ = false;
			StartEvent(pendingStrength_);
		}
		break;
	}
	case State::Warning:
		stateTime_ += dt;
		ApplyWeight(0.0f);
		if (stateTime_ >= warningTime_) { ChangeState(State::Rising); }
		break;
	case State::Rising:
		stateTime_ += dt;
		ApplyWeight(SmoothStep01(riseTime_ > 0.0f ? stateTime_ / riseTime_ : 1.0f) * strength_);
		if (stateTime_ >= riseTime_) { ChangeState(State::Peak); }
		break;
	case State::Peak:
		stateTime_ += dt;
		ApplyWeight(strength_);
		if (stateTime_ >= peakTime_) { ChangeState(State::Calming); }
		break;
	case State::Calming: {
		stateTime_ += dt;
		float t = SmoothStep01(calmTime_ > 0.0f ? stateTime_ / calmTime_ : 1.0f);
		ApplyWeight(calmFrom_ * (1.0f - t));
		if (stateTime_ >= calmTime_) {
			ApplyWeight(0.0f);
			ChangeState(State::Done);
		}
		break;
	}
	case State::Done:
		break;
	}
}

void RoughSeaComponent::ImGui() {
#ifdef USEIMGUI
	ImGui::Text("--- 海が荒れるイベント（うねり） ---");
	ImGui::TextDisabled("場所はコースエディタで W キー（コースに1つだけ）");

	if (ImGui::TreeNode("時間")) {
		ImGui::DragFloat("予告の時間", &warningTime_, 0.05f, 0.0f, 10.0f, "%.2f s");
		ImGui::DragFloat("大きくなる時間", &riseTime_, 0.05f, 0.1f, 10.0f, "%.2f s");
		ImGui::DragFloat("いちばん荒れている時間", &peakTime_, 0.1f, 0.1f, 60.0f, "%.1f s");
		ImGui::DragFloat("静まる時間", &calmTime_, 0.05f, 0.1f, 20.0f, "%.2f s");
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("メッセージ")) {
		char buf[512];
		strncpy_s(buf, noticeText_.c_str(), _TRUNCATE);
		if (ImGui::InputTextMultiline("告知の文（ステージセレクト）", buf, sizeof(buf))) { noticeText_ = buf; }
		strncpy_s(buf, warningText_.c_str(), _TRUNCATE);
		if (ImGui::InputTextMultiline("予告の文（通過したとき）", buf, sizeof(buf))) { warningText_ = buf; }
		ImGui::TreePop();
	}

	ImGui::Separator();
	ImGui::Text("状態: %s（%.2f 秒）  強さ: %.2f", kStateNames[static_cast<int>(state_)], stateTime_, weight_);
	ImGui::Text("船: %s  水面: %s  進行: %s  イルカ: %s",
		boat_ ? "あり" : "なし", water_ ? "あり" : "なし", flow_ ? "あり" : "なし", dolphin_ ? "あり" : "なし");
	if (pendingStart_) { ImGui::TextColored({ 1.0f, 0.8f, 0.2f, 1.0f }, "イルカの並走が終わるのを待っています"); }

	if (boat_) {
		int rough = boat_->GetRoughWaypoint();
		if (rough < 0) {
			ImGui::TextColored({ 1.0f, 0.5f, 0.3f, 1.0f }, "荒れる点がありません（コースエディタで W キー）");
		} else {
			ImGui::Text("荒れる点: %d（強さ %.2f）", rough, boat_->GetWaypoints()[rough].rough);
			// カーブするとうねりとの角度が変わって、山の来る間隔が変わる
			if (CalcTurnAfter(rough) > 0.2f) {
				ImGui::TextColored({ 1.0f, 0.85f, 0.2f, 1.0f }, "荒れる点の先でカーブしています。山の来る間隔が変わります");
			}
		}

		// 山がボーダーに来る間隔（出会い周期）の目安：T = 波長 / (船の速さ − うねりの速さ)
		if (water_) {
			float v = boat_->GetCruiseSpeed();
			for (int s = 0; s < WaterSurfaceComponent::kStormWaves; ++s) {
				float k = water_->GetStormWaveNumber(s);
				float length = 6.2831853f / k;
				float c = std::sqrt(9.8f / k);
				if (v > c + 0.1f) {
					ImGui::Text("うねり %d: 波長 %.0f m  山の来る間隔 %.2f 秒（巡航 %.0f m/s）", s + 1, length, length / (v - c), v);
				} else {
					ImGui::Text("うねり %d: 波長 %.0f m  船がうねりより遅いので追いつけない", s + 1, length);
				}
			}
			ImGui::TextDisabled("狙って跳べるのは 2〜3.5 秒くらい。水面の「荒れ用の波」の周波数で調整");
		}
	}

	// 確認用（Play 中に押す）
	if (ImGui::Button("今すぐ発生させる")) {
		if (FindReferences()) {
			float strength = 1.0f;
			int rough = boat_->GetRoughWaypoint();
			if (rough >= 0) { strength = boat_->GetWaypoints()[rough].rough; }
			StartEvent(strength);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("最初から")) {
		ApplyWeight(0.0f);
		state_ = State::Waiting;
		stateTime_ = 0.0f;
		hasPrevTarget_ = false;
		pendingStart_ = false;
	}
#endif
}

void RoughSeaComponent::Serialize(json& j) const {
	j["type"] = "RoughSeaComponent";
	j["warningTime"] = warningTime_;
	j["riseTime"] = riseTime_;
	j["peakTime"] = peakTime_;
	j["calmTime"] = calmTime_;
	j["noticeText"] = noticeText_;
	j["warningText"] = warningText_;
}

void RoughSeaComponent::Deserialize(const json& j) {
	if (j.contains("warningTime")) warningTime_ = j["warningTime"];
	if (j.contains("riseTime")) riseTime_ = j["riseTime"];
	if (j.contains("peakTime")) peakTime_ = j["peakTime"];
	if (j.contains("calmTime")) calmTime_ = j["calmTime"];
	if (j.contains("noticeText")) noticeText_ = j["noticeText"];
	if (j.contains("warningText")) warningText_ = j["warningText"];
}
