#include "PCH.h"
#include "DolphinEventComponent.h"
#include "BoatDriverComponent.h"
#include "BoarderComponent.h"
#include "GameFlowComponent.h"
#include "WaterSurfaceComponent.h"
#include "BoatWakeComponent.h"
#include "WaterQuery.h"
#include "GameObject.h"
#include "BaseScene.h"       // SceneContext を使うため
#include "ModelFactory.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "CameraOrganizer.h"
#include "RenderCollector.h"
#include "Model.h"
#include "MeshData.h"
#include "Time.h"
#include "MathFunction.h"
#include "LineRenderer.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

namespace {
	// 0〜1 をなめらかに（始めと終わりがゆっくり）
	float SmoothStep01(float t) {
		t = std::clamp(t, 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}
	float LerpF(float a, float b, float t) { return a + (b - a) * t; }

	std::mt19937& Rng() {
		static std::mt19937 rng{ std::random_device{}() };
		return rng;
	}

	const char* kStateNames[] = { "Waiting", "Approach", "Escort", "Leave", "Done" };
}

DolphinEventComponent::DolphinEventComponent() = default;

DolphinEventComponent::~DolphinEventComponent() {
	WaterQuery::FreeSlot(querySlot_);
}

bool DolphinEventComponent::FindReferences() {
	if (!boat_) { boat_ = gameObject_->FindComponentInScene<BoatDriverComponent>(); }
	if (!wake_) { wake_ = gameObject_->FindComponentInScene<BoatWakeComponent>(); }
	if (!boarder_) { boarder_ = gameObject_->FindComponentInScene<BoarderComponent>(); }
	if (!flow_) { flow_ = gameObject_->FindComponentInScene<GameFlowComponent>(); }
	// ボーダーと GameFlow は無くても動く（ボーダーが無ければロープの長さは仮の値、GameFlow が無ければ常に Playing 扱い）
	return boat_ && wake_;
}

void DolphinEventComponent::ChooseWaypoint() {
	isChosen_ = true;

	const auto& wps = boat_->GetWaypoints();
	std::vector<int> candidates;

	// コースエディタで印を付けた点（道中の点だけ）
	for (int i = 0; i < static_cast<int>(wps.size()); ++i) {
		if (wps[i].dolphin && wps[i].type == BoatDriverComponent::WaypointType::Pass) {
			candidates.push_back(i);
		}
	}
	// 印が1つも無いとき：道中の点から。ただし次がゴールの点（ゴール直前）は並走の時間が取れないので除く
	if (candidates.empty()) {
		for (int i = 0; i < static_cast<int>(wps.size()); ++i) {
			if (wps[i].type != BoatDriverComponent::WaypointType::Pass) continue;
			bool nextIsGoal = (i + 1 < static_cast<int>(wps.size())) && wps[i + 1].type == BoatDriverComponent::WaypointType::Goal;
			if (nextIsGoal) continue;
			candidates.push_back(i);
		}
	}

	if (candidates.empty()) {
		chosenWaypoint_ = -1; // ウェイポイントの無いコース：fallbackDelay_ 秒で来る
	} else {
		std::uniform_int_distribution<size_t> pick(0, candidates.size() - 1);
		chosenWaypoint_ = candidates[pick(Rng())];
	}
}

void DolphinEventComponent::ChangeState(State next) {
	// Leave は今いる場所から離れ始める（並走の途中でゴールしたときなども、瞬間移動しないように）
	if (next == State::Leave) {
		leaveFromForward_ = lastForward_;
		leaveFromLateral_ = lastLateral_;
		leaveFromDepth_ = lastDepth_;
		leaveFromWeight_ = lastWaveWeight_;
	}
	state_ = next;
	stateTime_ = 0.0f;
}

float DolphinEventComponent::DecideSide() const {
	if (side_ != 0) { return (side_ > 0) ? 1.0f : -1.0f; }

	// 抽選した点 k と、その次の点 k+1 で曲がる量を足す（並走している間に曲がる量）
	// yaw は atan2(x, z) なので、増える = 右に曲がる
	float turn = 0.0f;
	if (boat_ && chosenWaypoint_ >= 0) {
		const size_t count = boat_->GetWaypoints().size();
		auto yawOf = [&](size_t i) {
			Vector3 d = boat_->GetIncomingDir(i);
			return std::atan2(d.x, d.z);
		};
		auto wrap = [](float a) { return std::remainder(a, 6.2831853f); }; // -π〜π
		size_t k = static_cast<size_t>(chosenWaypoint_);
		if (k + 1 < count) { turn += wrap(yawOf(k + 1) - yawOf(k)); }
		if (k + 2 < count) { turn += wrap(yawOf(k + 2) - yawOf(k + 1)); }
	}

	// 右に曲がる → 外側は左
	const float kStraight = 0.1f; // これより小さければ直線とみなす[rad]（約6°）
	if (turn > kStraight) return -1.0f;
	if (turn < -kStraight) return 1.0f;

	// 直線：ランダム
	std::bernoulli_distribution coin(0.5);
	return coin(Rng()) ? 1.0f : -1.0f;
}

void DolphinEventComponent::StartEvent() {
	sideSign_ = DecideSide();
	hasPrevXZ_ = false;
	swimTime_ = 0.0f;
	ChangeState(State::Approach);
}

float DolphinEventComponent::CalcWaveSpeed() const {
	// Ripple.CS：h += 速度 + waveSpeed(0.7) × 0.25 × ラプラシアン → 1フレームに √0.175 ≒ 0.418 マス進む
	// シェーダーの waveSpeed を変えたら、ここも合わせる
	constexpr float kTexelsPerFrame = 0.4183f;
	constexpr float kSimResolution = 512.0f; // BoatWakeComponent の kSimRes
	float texel = wake_ ? wake_->GetSimSize().x / kSimResolution : 150.0f / kSimResolution;
	return kTexelsPerFrame * texel / (std::max)(smoothedDt_, 1e-4f) * waveSpeedScale_;
}

float DolphinEventComponent::CalcWakeTan() const {
	float speed = boat_ ? boat_->GetSpeed() : 0.0f;
	float waveSpeed = CalcWaveSpeed();
	if (speed <= waveSpeed + 0.1f) return 0.0f; // 波より遅いと V 字にならない
	float s = waveSpeed / speed;                 // sinθ
	return s / std::sqrt(1.0f - s * s);           // tanθ
}

float DolphinEventComponent::CalcCrossDistance() const {
	float rope = boarder_ ? boarder_->GetRopeLength() : 30.0f;
	float stern = boarder_ ? boarder_->GetSternOffset() : 2.0f;
	float t = CalcWakeTan();

	// 船の航跡の腕の上の点 (x, x·tanθ) が、船尾から rope の距離にある x を求める
	// (x - stern)² + (x·t)² = rope²  →  (1 + t²)x² - 2·stern·x + stern² - rope² = 0
	float a = 1.0f + t * t;
	float b = -2.0f * stern;
	float c = stern * stern - rope * rope;
	float disc = (std::max)(b * b - 4.0f * a * c, 0.0f);
	float x = (-b + std::sqrt(disc)) / (2.0f * a);
	return (std::max)(x + crossAdjust_, 1.0f);
}

float DolphinEventComponent::CalcEscortLateral() const {
	float t = CalcWakeTan();
	if (t <= 0.0f) return 15.0f; // V ができないとき（加速中など）の仮の間隔
	return 2.0f * CalcCrossDistance() * t;
}

void DolphinEventComponent::EvaluatePose(float& forward, float& lateral, float& depth, float& waveWeight) const {
	float escortLateral = CalcEscortLateral();

	switch (state_) {
	case State::Approach: {
		// 斜め前の外側から、並走の位置へ寄ってくる。波も少しずつ強くなる
		float t = SmoothStep01(approachTime_ > 0.0f ? stateTime_ / approachTime_ : 1.0f);
		forward = LerpF(approachAhead_, 0.0f, t);
		lateral = LerpF(escortLateral + approachFrom_, escortLateral, t);
		depth = LerpF(hiddenDepth_, swimDepth_, t);
		waveWeight = t;
		break;
	}
	case State::Escort:
		forward = 0.0f; // 船と横に並ぶ（V 字の先が揃うので、計算どおりの位置で交わる）
		lateral = escortLateral;
		depth = swimDepth_;
		waveWeight = 1.0f;
		break;
	case State::Leave: {
		float t = SmoothStep01(leaveTime_ > 0.0f ? stateTime_ / leaveTime_ : 1.0f);
		forward = LerpF(leaveFromForward_, leaveAhead_, t);
		lateral = LerpF(leaveFromLateral_, escortLateral + leaveTo_, t);
		depth = LerpF(leaveFromDepth_, hiddenDepth_, t);
		waveWeight = LerpF(leaveFromWeight_, 0.0f, t);
		break;
	}
	default:
		forward = 0.0f;
		lateral = escortLateral;
		depth = hiddenDepth_;
		waveWeight = 0.0f;
		break;
	}
}

void DolphinEventComponent::Update() {
	if (!gameObject_) return;
	if (!FindReferences()) return;

	float dt = Time::GetDeltaTime();
	if (dt <= 0.0f) return;
	smoothedDt_ += (dt - smoothedDt_) * 0.05f; // FPS のぶれをならす

	// ステージ開始時に1回だけ抽選（リトライはシーンを作り直すので、毎回引き直しになる）
	if (!isChosen_) { ChooseWaypoint(); }

	using Phase = GameFlowComponent::Phase;
	bool isPlaying = !flow_ || flow_->GetPhase() == Phase::Playing;

	// ゴール・ジャンプ終了したら、並走中でも帰っていく
	if (!isPlaying && (state_ == State::Approach || state_ == State::Escort)) {
		ChangeState(State::Leave);
	}

	// ===== 状態を進める =====
	switch (state_) {
	case State::Waiting: {
		if (!isPlaying) break;
		playTime_ += dt;

		if (chosenWaypoint_ >= 0) {
			// 船が抽選した点を通過した（向かう点がその点から次へ変わった）
			size_t target = boat_->GetTargetIndex();
			if (hasPrevTarget_ && prevTarget_ == static_cast<size_t>(chosenWaypoint_) && target != prevTarget_) {
				StartEvent();
			}
			prevTarget_ = target;
			hasPrevTarget_ = true;
		} else if (playTime_ >= fallbackDelay_) {
			StartEvent();
		}
		break;
	}
	case State::Approach:
		stateTime_ += dt;
		if (stateTime_ >= approachTime_) { ChangeState(State::Escort); }
		break;
	case State::Escort:
		stateTime_ += dt;
		if (stateTime_ >= escortTime_) { ChangeState(State::Leave); }
		break;
	case State::Leave:
		stateTime_ += dt;
		if (stateTime_ >= leaveTime_) { ChangeState(State::Done); }
		break;
	case State::Done:
		break;
	}

	if (state_ != State::Approach && state_ != State::Escort && state_ != State::Leave) {
		lastWaveWeight_ = 0.0f;
		UpdateModel();
		return;
	}

	// ===== 船から見た位置 → ワールド位置 =====
	float forward, lateral, depth, weight;
	EvaluatePose(forward, lateral, depth, weight);
	lastForward_ = forward;
	lastLateral_ = lateral;
	lastDepth_ = depth;
	lastWaveWeight_ = weight;

	const Vector3& boatPos = boat_->GetGameObject()->GetTransform().translate;
	float boatYaw = boat_->GetYaw();
	Vector2 fwd = { std::sin(boatYaw), std::cos(boatYaw) };
	Vector2 right = { fwd.y, -fwd.x }; // BoarderComponent と同じ右方向
	Vector2 xz = Vector2{ boatPos.x, boatPos.z } + fwd * forward + right * (lateral * sideSign_);

	// 水平速度（波源に渡す。船と同じ速さで動くので、船と同じ形の V 字ができる）
	if (hasPrevXZ_) {
		velocity_ = (xz - prevXZ_) / dt;
	} else {
		velocity_ = fwd * boat_->GetSpeed();
	}
	prevXZ_ = xz;
	hasPrevXZ_ = true;

	// ===== 水面の高さ =====
	float waterY = 0.0f;
	if (auto* water = gameObject_->FindComponentInScene<WaterSurfaceComponent>()) {
		waterY = water->SampleGerstnerHeight(xz.x, xz.y);
	}
	if (querySlot_ < 0) { querySlot_ = WaterQuery::AllocateSlot(); }
	if (querySlot_ >= 0) {
		auto& query = wake_->GetWaterQuery();
		uint32_t slot = static_cast<uint32_t>(querySlot_);
		query.SetQueryPoint(slot, xz.x, xz.y);
		waterY += query.GetResult(slot).height;
	}

	// ===== 泳ぎ（上下にうねりながら進む） =====
	swimTime_ += dt;
	float phase = swimTime_ * porpoiseSpeed_;
	float bob = std::sin(phase) * porpoiseHeight_;
	position_ = { xz.x, waterY - depth + bob, xz.y };

	// 向き：進んでいる方向（寄ってくる・離れるときは少し斜めを向く）。頭は上下の動きに合わせる
	if (Math::Length(velocity_) > 1.0f) {
		yaw_ = std::atan2(velocity_.x, velocity_.y);
	} else {
		yaw_ = boatYaw;
	}
	pitch_ = std::cos(phase) * porpoisePitch_;

	// ===== 波を立てる =====
	if (weight > 0.001f) {
		WaveSource src{};
		src.position = xz;
		src.velocity = velocity_;
		src.radius = waveRadius_;
		src.strength = waveStrength_ * weight;
		src.foam = foam_ * weight;
		wake_->AddWaveSource(src);
	}

	// Play 中は GameObject::UpdateTransformBuffer が呼ばれないので、ここでモデルの行列も更新する
	// （CourseBuoyComponent と同じ。カメラは前フレームの値になる）
	UpdateModel();
}

void DolphinEventComponent::UpdateModel() {
	if (!gameObject_) return;

	if (!model_) {
		SceneContext* context = gameObject_->GetContext();
		if (!context) return;
		textureIndex_ = context->textureManager->LoadTexture(texturePath_);
		uint32_t modelIndex = context->modelManager->LoadModelData(modelPath_);
		model_ = context->modelFactory->CreateModel(modelIndex, textureIndex_);
		if (!model_) return;
	}

	// 回転は X → Y → Z の順。体の X（頭の上げ下げ）を先に回してから Y（向き）で回すので、1つの Transform で足りる
	// （X 軸まわりの + 回転で +Z の頭が下がるので、頭を上げるときはマイナス）
	model_->SetTranslate(position_);
	model_->SetRotate({ -pitch_, yaw_, 0.0f });
	model_->SetScale({ scale_, scale_, scale_ });
	const CameraData& cameraData = CameraOrganizer::GetInstance()->GetCameraData();
	model_->Update(&cameraData);
}

void DolphinEventComponent::CollectRenderData(MyEngine::Rendering::RenderCollector* collector) const {
	if (!isVisible_ || !collector || !model_) return;
	if (state_ != State::Approach && state_ != State::Escort && state_ != State::Leave) return;

	auto* modelData = model_->GetModelData();
	auto* material = model_->GetMaterial();
	if (!modelData || !material) return;

	for (size_t i = 0; i < modelData->meshes.size(); ++i) {
		MyEngine::Rendering::RenderItem item{};
		item.mesh = &modelData->meshes[i];
		item.material = material;
		item.transformAddr = model_->GetMeshTransformGPUAddress(static_cast<uint32_t>(i));
		item.customAddr = 0;
		item.debugName = "Dolphin";
		collector->Submit(item);
	}
}

void DolphinEventComponent::DrawDebug() {
#ifdef USEIMGUI
	if (!gameObject_ || !boat_) return;
	auto* line = LineRenderer::GetInstance();
	const Vector4 kWaitColor = { 0.2f, 0.9f, 1.0f, 1.0f };   // 抽選された点
	const Vector4 kArmColor = { 0.2f, 0.9f, 1.0f, 0.8f };    // 航跡の腕（予想）
	const Vector4 kCrossColor = { 1.0f, 0.3f, 0.8f, 1.0f };  // 交わる点

	// 抽選された点（シアンの高い柱）
	const auto& wps = boat_->GetWaypoints();
	if (state_ == State::Waiting && chosenWaypoint_ >= 0 && chosenWaypoint_ < static_cast<int>(wps.size())) {
		const Vector3& p = wps[chosenWaypoint_].pos;
		line->DrawLine({ p.x, 0.5f, p.z }, { p.x, 14.0f, p.z }, kWaitColor, false);
		line->DrawCircleXZ({ p.x, 0.5f, p.z }, 4.0f, kWaitColor, 24, false);
	}

	if (state_ != State::Approach && state_ != State::Escort && state_ != State::Leave) return;

	// 予想した航跡の腕と、交わる点。実際の白い航跡と見比べて Wave Speed Scale を合わせる
	const Vector3& boatPos = boat_->GetGameObject()->GetTransform().translate;
	float yaw = boat_->GetYaw();
	Vector2 fwd = { std::sin(yaw), std::cos(yaw) };
	Vector2 right = Vector2{ fwd.y, -fwd.x } * sideSign_; // イルカ側
	float t = CalcWakeTan();
	float x = CalcCrossDistance();
	float len = x * 1.5f;
	const float y = 0.5f;

	auto toV3 = [&](const Vector2& p) { return Vector3{ p.x, y, p.y }; };
	Vector2 boatXZ = { boatPos.x, boatPos.z };
	Vector2 dolphinXZ = { position_.x, position_.z };

	// 船の腕（イルカ側）と、イルカの内側の腕
	line->DrawLine(toV3(boatXZ), toV3(boatXZ - fwd * len + right * (len * t)), kArmColor, false);
	line->DrawLine(toV3(dolphinXZ), toV3(dolphinXZ - fwd * len - right * (len * t)), kArmColor, false);

	// 交わる点（イルカが横に並んでいるときの位置）
	Vector2 cross = boatXZ - fwd * x + right * (x * t);
	line->DrawCircleXZ(toV3(cross), 3.0f, kCrossColor, 24, false);
	line->DrawLine(toV3(cross), { cross.x, y + 6.0f, cross.y }, kCrossColor, false);
#endif
}

void DolphinEventComponent::ImGui() {
#ifdef USEIMGUI
	ImGui::Text("--- イルカの大波イベント ---");
	ImGui::Checkbox("Visible", &isVisible_);
	ImGui::DragFloat("Scale", &scale_, 0.05f, 0.1f, 20.0f);
	ImGui::SliderInt("Side (1 R / -1 L / 0 Auto)", &side_, -1, 1);

	if (ImGui::TreeNode("Position")) {
		ImGui::DragFloat("Wave Speed Scale", &waveSpeedScale_, 0.01f, 0.2f, 5.0f);
		ImGui::Text("wave speed: %.1f m/s (%.0f FPS)", CalcWaveSpeed(), 1.0f / (std::max)(smoothedDt_, 1e-4f));
		ImGui::DragFloat("Cross Adjust", &crossAdjust_, 0.1f, -50.0f, 50.0f, "%.1f m");
		ImGui::DragFloat("Approach From", &approachFrom_, 0.5f, 0.0f, 200.0f, "%.1f m");
		ImGui::DragFloat("Approach Ahead", &approachAhead_, 0.5f, -100.0f, 200.0f, "%.1f m");
		ImGui::DragFloat("Leave To", &leaveTo_, 0.5f, 0.0f, 200.0f, "%.1f m");
		ImGui::DragFloat("Leave Ahead", &leaveAhead_, 0.5f, -100.0f, 200.0f, "%.1f m");
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Time")) {
		ImGui::DragFloat("Approach Time", &approachTime_, 0.05f, 0.1f, 20.0f, "%.2f s");
		ImGui::DragFloat("Escort Time", &escortTime_, 0.05f, 0.1f, 60.0f, "%.2f s");
		ImGui::DragFloat("Leave Time", &leaveTime_, 0.05f, 0.1f, 20.0f, "%.2f s");
		ImGui::DragFloat("Fallback Delay", &fallbackDelay_, 0.1f, 0.0f, 120.0f, "%.1f s");
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Wave")) {
		ImGui::DragFloat("Strength", &waveStrength_, 0.01f, 0.0f, 5.0f);
		ImGui::DragFloat("Radius", &waveRadius_, 0.05f, 0.5f, 20.0f, "%.2f m");
		ImGui::DragFloat("Foam", &foam_, 0.001f, 0.0f, 1.0f, "%.3f");
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Swim")) {
		ImGui::DragFloat("Swim Depth", &swimDepth_, 0.01f, -2.0f, 5.0f, "%.2f m");
		ImGui::DragFloat("Hidden Depth", &hiddenDepth_, 0.05f, 0.0f, 20.0f, "%.2f m");
		ImGui::DragFloat("Porpoise Height", &porpoiseHeight_, 0.01f, 0.0f, 5.0f, "%.2f m");
		ImGui::DragFloat("Porpoise Speed", &porpoiseSpeed_, 0.05f, 0.0f, 20.0f, "%.2f rad/s");
		ImGui::DragFloat("Porpoise Pitch", &porpoisePitch_, 0.01f, 0.0f, 1.5f, "%.2f rad");
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Notice")) {
		// ステージセレクトに出す予告文（どこで来るかは書かない）
		char buf[512];
		strncpy_s(buf, noticeText_.c_str(), _TRUNCATE);
		if (ImGui::InputTextMultiline("Notice Text", buf, sizeof(buf))) { noticeText_ = buf; }
		ImGui::TreePop();
	}

	ImGui::Separator();
	ImGui::Text("state: %s (%.2f s)", kStateNames[static_cast<int>(state_)], stateTime_);
	ImGui::Text("waypoint: %d  side: %s", chosenWaypoint_, sideSign_ > 0.0f ? "right" : "left");
	float t = boat_ ? CalcWakeTan() : 0.0f;
	ImGui::Text("wake angle: %.1f deg  cross: %.1f m behind  lateral: %.1f m",
		std::atan(t) * 57.2958f, boat_ ? CalcCrossDistance() : 0.0f, boat_ ? CalcEscortLateral() : 0.0f);
	ImGui::Text("wave weight: %.2f  query slot: %d", lastWaveWeight_, querySlot_);
	ImGui::Text("boat: %s  wake: %s  boarder: %s  flow: %s",
		boat_ ? "found" : "none", wake_ ? "found" : "none", boarder_ ? "found" : "none", flow_ ? "found" : "none");

	// 確認用（Play 中に押す）
	if (ImGui::Button("Trigger Now")) { StartEvent(); }
	ImGui::SameLine();
	if (ImGui::Button("Re-roll")) {
		if (boat_) { ChooseWaypoint(); }
		state_ = State::Waiting;
		hasPrevTarget_ = false;
	}
#endif
}

void DolphinEventComponent::Serialize(json& j) const {
	j["type"] = "DolphinEventComponent";
	j["isVisible"] = isVisible_;
	j["modelPath"] = modelPath_;
	j["texturePath"] = texturePath_;
	j["scale"] = scale_;
	j["side"] = side_;
	j["waveSpeedScale"] = waveSpeedScale_;
	j["crossAdjust"] = crossAdjust_;
	j["approachFrom"] = approachFrom_;
	j["approachAhead"] = approachAhead_;
	j["leaveTo"] = leaveTo_;
	j["leaveAhead"] = leaveAhead_;
	j["approachTime"] = approachTime_;
	j["escortTime"] = escortTime_;
	j["leaveTime"] = leaveTime_;
	j["fallbackDelay"] = fallbackDelay_;
	j["waveStrength"] = waveStrength_;
	j["waveRadius"] = waveRadius_;
	j["foam"] = foam_;
	j["swimDepth"] = swimDepth_;
	j["hiddenDepth"] = hiddenDepth_;
	j["porpoiseHeight"] = porpoiseHeight_;
	j["porpoiseSpeed"] = porpoiseSpeed_;
	j["porpoisePitch"] = porpoisePitch_;
	j["noticeText"] = noticeText_;
}

void DolphinEventComponent::Deserialize(const json& j) {
	if (j.contains("isVisible")) isVisible_ = j["isVisible"];
	if (j.contains("modelPath")) modelPath_ = j["modelPath"];
	if (j.contains("texturePath")) texturePath_ = j["texturePath"];
	if (j.contains("scale")) scale_ = j["scale"];
	if (j.contains("side")) side_ = j["side"];
	if (j.contains("waveSpeedScale")) waveSpeedScale_ = j["waveSpeedScale"];
	if (j.contains("crossAdjust")) crossAdjust_ = j["crossAdjust"];
	if (j.contains("approachFrom")) approachFrom_ = j["approachFrom"];
	if (j.contains("approachAhead")) approachAhead_ = j["approachAhead"];
	if (j.contains("leaveTo")) leaveTo_ = j["leaveTo"];
	if (j.contains("leaveAhead")) leaveAhead_ = j["leaveAhead"];
	if (j.contains("approachTime")) approachTime_ = j["approachTime"];
	if (j.contains("escortTime")) escortTime_ = j["escortTime"];
	if (j.contains("leaveTime")) leaveTime_ = j["leaveTime"];
	if (j.contains("fallbackDelay")) fallbackDelay_ = j["fallbackDelay"];
	if (j.contains("waveStrength")) waveStrength_ = j["waveStrength"];
	if (j.contains("waveRadius")) waveRadius_ = j["waveRadius"];
	if (j.contains("foam")) foam_ = j["foam"];
	if (j.contains("swimDepth")) swimDepth_ = j["swimDepth"];
	if (j.contains("hiddenDepth")) hiddenDepth_ = j["hiddenDepth"];
	if (j.contains("porpoiseHeight")) porpoiseHeight_ = j["porpoiseHeight"];
	if (j.contains("porpoiseSpeed")) porpoiseSpeed_ = j["porpoiseSpeed"];
	if (j.contains("porpoisePitch")) porpoisePitch_ = j["porpoisePitch"];
	if (j.contains("noticeText")) noticeText_ = j["noticeText"];
}
