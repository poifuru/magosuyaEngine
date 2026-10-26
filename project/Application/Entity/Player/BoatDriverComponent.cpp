#include "PCH.h"
#include "BoatDriverComponent.h"
#include "GameObject.h"
#include "Time.h"
#include "LineRenderer.h"
#include "CameraOrganizer.h"
#include "MathFunction.h"
#ifdef USEIMGUI
#include "EditorManager.h"
#include "GameViewWindow.h"
#include <imGuizmo.h>
#endif
#include <algorithm>
#include <cmath>

namespace {
	constexpr float kPi = 3.14159265f;
	constexpr float kTwoPi = 6.2831853f;

	// 角度を -π ～ π に収める
	float WrapAngle(float a) {
		while (a > kPi) a -= kTwoPi;
		while (a < -kPi) a += kTwoPi;
		return a;
	}

	// 行列を掛けて w で割る（射影行列・その逆行列用）
	bool TransformCoord(const Vector3& v, const Matrix4x4& m, Vector3& out) {
		float x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
		float y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
		float z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
		float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];
		if (std::abs(w) < 1e-6f) return false;
		out = { x / w, y / w, z / w };
		return w > 0.0f; // カメラの後ろ
	}

	// 点 p から線分 ab までの距離の2乗（XZ平面）
	float DistSqToSegmentXZ(const Vector3& p, const Vector3& a, const Vector3& b) {
		float abx = b.x - a.x, abz = b.z - a.z;
		float len2 = abx * abx + abz * abz;
		float t = (len2 > 1e-6f) ? ((p.x - a.x) * abx + (p.z - a.z) * abz) / len2 : 0.0f;
		t = std::clamp(t, 0.0f, 1.0f);
		float dx = a.x + abx * t - p.x, dz = a.z + abz * t - p.z;
		return dx * dx + dz * dz;
	}

	// 色
	const Vector4 kStartColor = { 1.0f, 0.85f, 0.1f, 1.0f };    // スタート地点
	const Vector4 kHeadingColor = { 0.2f, 0.9f, 1.0f, 1.0f };   // 今の進行方向
	const Vector4 kPointColor = { 1.0f, 0.45f, 0.1f, 1.0f };    // ウェイポイント
	const Vector4 kFirstColor = { 0.3f, 1.0f, 0.3f, 1.0f };     // 最初の点
	const Vector4 kSelectColor = { 1.0f, 1.0f, 1.0f, 1.0f };    // 選択・マウスが乗っている点
	const Vector4 kCourseColor = { 1.0f, 0.45f, 0.1f, 0.8f };   // 点をつなぐ線
	const Vector4 kRadiusColor = { 1.0f, 0.45f, 0.1f, 0.25f };  // 到着判定の円
	const Vector4 kPreviewColor = { 0.4f, 1.0f, 0.6f, 0.9f };   // 実際に走る線
	const Vector4 kTargetColor = { 1.0f, 0.2f, 0.2f, 1.0f };    // 今向かっている点

	constexpr float kLineY = 0.3f; // 線を描く高さ（水面に埋もれにくく）
}

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
	targetIndex_ = 0;
}

bool BoatDriverComponent::SteerStep(const Vector3& pos, float& yaw, size_t& target, float dt) const {
	if (waypoints_.empty()) return true;            // コースなし：まっすぐ
	if (target >= waypoints_.size()) return false;  // 走り終えた：まっすぐ

	// 近づいたら次の点へ
	const Vector3& wp = waypoints_[target];
	float dx = wp.x - pos.x;
	float dz = wp.z - pos.z;
	if (dx * dx + dz * dz < arriveRadius_ * arriveRadius_) {
		target++;
		if (target >= waypoints_.size()) {
			if (!loop_) return false;
			target = 0;
		}
		dx = waypoints_[target].x - pos.x;
		dz = waypoints_[target].z - pos.z;
	}

	// 目標の方向との差を、1ステップで曲がれる量までに制限して回す
	float targetYaw = std::atan2(dx, dz);
	float diff = WrapAngle(targetYaw - yaw);
	float maxStep = maxTurnRate_ * dt;
	yaw = WrapAngle(yaw + std::clamp(diff, -maxStep, maxStep));
	return true;
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

	// ウェイポイントへ向けて舵を切る（止まっているときは曲がらない）
	if (speed_ > 0.01f) {
		SteerStep(t.translate, yaw_, targetIndex_, dt);
	}

	// 進行方向へ進める（BoarderComponent と同じく yaw = atan2(x, z) の向き）
	float step = speed_ * dt;
	t.translate.x += std::sin(yaw_) * step;
	t.translate.z += std::cos(yaw_) * step;
	t.translate.y = boatHeight_;
	t.rotate.y = yaw_;

	distance_ += step;
}

void BoatDriverComponent::DrawDebug() {
	if (!gameObject_) return;
	auto* line = LineRenderer::GetInstance();
	const auto& t = gameObject_->GetTransform();

#ifdef USEIMGUI
	// エディタ停止中は、ギズモで動かした船の位置と向きをそのままスタート地点にする
	if (EditorManager::GetInstance()->GetPlayState() == EditorPlayState::Edit) {
		startPos_ = t.translate;
		startYaw_ = t.rotate.y;
		yaw_ = startYaw_;
		targetIndex_ = 0;
	}
#endif

	// スタート地点（黄色の円と、スタート時の向き）
	Vector3 start = { startPos_.x, kLineY, startPos_.z };
	line->DrawCircleXZ(start, 3.0f, kStartColor, 32, false);
	line->DrawArrow(start, { start.x + std::sin(startYaw_) * 6.0f, kLineY, start.z + std::cos(startYaw_) * 6.0f }, kStartColor, 0.3f, false);

	// 今の進行方向（シアンの矢印、長さ = 1秒で進む距離）
	float len = std::max(speed_, 3.0f);
	Vector3 from = { t.translate.x, t.translate.y + 1.0f, t.translate.z };
	line->DrawArrow(from, { from.x + std::sin(yaw_) * len, from.y, from.z + std::cos(yaw_) * len }, kHeadingColor, 0.2f, false);

	// --- コース ---
	const size_t count = waypoints_.size();
	for (size_t i = 0; i < count; ++i) {
		Vector3 p = { waypoints_[i].x, kLineY, waypoints_[i].z };

		// 点をつなぐ線（ループなら最後から最初へも）
		if (i + 1 < count) {
			line->DrawLine(p, { waypoints_[i + 1].x, kLineY, waypoints_[i + 1].z }, kCourseColor, false);
		} else if (loop_ && count >= 2) {
			Vector4 c = kCourseColor;
			c.w *= 0.4f; // 戻りの線は薄く
			line->DrawLine(p, { waypoints_[0].x, kLineY, waypoints_[0].z }, c, false);
		}

		// 点そのもの（ブイのような縦棒 + 円）
		bool isHot = (static_cast<int>(i) == selectedIndex_) || (static_cast<int>(i) == hoverIndex_);
		Vector4 color = isHot ? kSelectColor : (i == 0 ? kFirstColor : kPointColor);
		float r = isHot ? 2.0f : 1.5f;
		line->DrawCircleXZ(p, r, color, 16, false);
		line->DrawLine(p, { p.x, p.y + 4.0f, p.z }, color, false);

		// 到着判定の範囲
		line->DrawCircleXZ(p, arriveRadius_, kRadiusColor, 48, false);
	}

	// 今向かっている点（赤い縦棒）
	if (targetIndex_ < count) {
		const Vector3& wp = waypoints_[targetIndex_];
		line->DrawLine({ wp.x, kLineY, wp.z }, { wp.x, kLineY + 8.0f, wp.z }, kTargetColor, false);
	}

	// 実際に走る線
	if (showPreview_ && count > 0) {
		DrawPreviewPath();
	}
}

void BoatDriverComponent::DrawPreviewPath() const {
	auto* line = LineRenderer::GetInstance();

	// Update と同じ計算を、スタート地点から 0.1 秒刻みで先回りして回す
	const float dt = 0.1f;
	const int kMaxSteps = 3000; // 最大 300 秒ぶん
	Vector3 pos = { startPos_.x, kLineY, startPos_.z };
	float yaw = startYaw_;
	float speed = 0.0f;
	size_t target = 0;
	bool visitedLast = false;

	for (int i = 0; i < kMaxSteps; ++i) {
		speed = std::min(speed + acceleration_ * dt, cruiseSpeed_);
		if (speed <= 0.0f) break;

		size_t before = target;
		bool running = SteerStep(pos, yaw, target, dt);

		Vector3 next = { pos.x + std::sin(yaw) * speed * dt, kLineY, pos.z + std::cos(yaw) * speed * dt };
		line->DrawLine(pos, next, kPreviewColor, false);
		pos = next;

		// 1周したら（最後の点を過ぎて最初の点に戻ったら）終わり
		if (before == waypoints_.size() - 1 && target != before) visitedLast = true;
		if (!running || (visitedLast && loop_)) break;
	}
}

void BoatDriverComponent::ResetToStart() {
	if (!gameObject_) return;
	auto& t = gameObject_->GetTransform();
	t.translate = startPos_;
	t.rotate.y = startYaw_;
	yaw_ = startYaw_;
	speed_ = 0.0f;
	distance_ = 0.0f;
	targetIndex_ = 0;
}

#ifdef USEIMGUI
void BoatDriverComponent::HandleWaypointEditing() {
	hoverIndex_ = -1;

	auto* gameView = EditorManager::GetInstance()->GetWindow<GameViewWindow>();
	if (!gameView) return;

	// ドラッグ中にボタンが離されたら終わり
	if (dragIndex_ >= 0 && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		dragIndex_ = -1;
	}

	ImVec2 rectPos = gameView->GetGameScreenPos();
	ImVec2 rectSize = gameView->GetGameScreenSize();
	if (rectSize.x <= 0.0f || rectSize.y <= 0.0f) return;

	ImVec2 mouse = ImGui::GetMousePos();
	bool insideRect = mouse.x >= rectPos.x && mouse.x <= rectPos.x + rectSize.x &&
		mouse.y >= rectPos.y && mouse.y <= rectPos.y + rectSize.y;
	if (!insideRect && dragIndex_ < 0) return;
	if (!gameView->IsHovered() && dragIndex_ < 0) return;

	const Matrix4x4& vp = CameraOrganizer::GetInstance()->GetCameraData().vp;

	// --- ワールド → スクリーン（点の上にマウスがあるか） ---
	auto worldToScreen = [&](const Vector3& p, ImVec2& out) {
		Vector3 ndc;
		if (!TransformCoord(p, vp, ndc)) return false;
		out.x = rectPos.x + (ndc.x * 0.5f + 0.5f) * rectSize.x;
		out.y = rectPos.y + (0.5f - ndc.y * 0.5f) * rectSize.y;
		return true;
	};

	// --- スクリーン → 水面（y = 0 の平面との交点） ---
	auto screenToWater = [&](Vector3& hit) {
		float ndcX = (mouse.x - rectPos.x) / rectSize.x * 2.0f - 1.0f;
		float ndcY = 1.0f - (mouse.y - rectPos.y) / rectSize.y * 2.0f;
		Matrix4x4 invVP = Math::Inverse(vp);
		Vector3 nearP, farP;
		TransformCoord({ ndcX, ndcY, 0.0f }, invVP, nearP);
		TransformCoord({ ndcX, ndcY, 1.0f }, invVP, farP);
		Vector3 dir = { farP.x - nearP.x, farP.y - nearP.y, farP.z - nearP.z };
		if (std::abs(dir.y) < 1e-6f) return false;   // 水平に見ている
		float t = (0.0f - nearP.y) / dir.y;
		if (t < 0.0f) return false;                   // 水面がカメラの後ろ
		hit = { nearP.x + dir.x * t, 0.0f, nearP.z + dir.z * t };
		return true;
	};

	// マウスが乗っている点（画面上で 12px 以内の一番近い点）
	float bestDist = 12.0f * 12.0f;
	for (size_t i = 0; i < waypoints_.size(); ++i) {
		ImVec2 s;
		if (!worldToScreen({ waypoints_[i].x, kLineY, waypoints_[i].z }, s)) continue;
		float dx = s.x - mouse.x, dy = s.y - mouse.y;
		float d = dx * dx + dy * dy;
		if (d < bestDist) {
			bestDist = d;
			hoverIndex_ = static_cast<int>(i);
		}
	}

	// ギズモを触っているときはクリックを横取りしない
	bool gizmoBusy = ImGuizmo::IsOver() || ImGuizmo::IsUsing();

	// 左クリック：点の上なら掴む、何もなければ追加（Shift なら一番近い線の間に挿入）
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !gizmoBusy && insideRect) {
		if (hoverIndex_ >= 0) {
			dragIndex_ = hoverIndex_;
			selectedIndex_ = hoverIndex_;
		} else {
			Vector3 hit;
			if (screenToWater(hit)) {
				size_t insertAt = waypoints_.size();
				if (ImGui::GetIO().KeyShift && waypoints_.size() >= 2) {
					// 一番近い線分の後ろに入れる
					float best = 1e30f;
					size_t segCount = loop_ ? waypoints_.size() : waypoints_.size() - 1;
					for (size_t i = 0; i < segCount; ++i) {
						const Vector3& a = waypoints_[i];
						const Vector3& b = waypoints_[(i + 1) % waypoints_.size()];
						float d = DistSqToSegmentXZ(hit, a, b);
						if (d < best) {
							best = d;
							insertAt = i + 1;
						}
					}
				}
				waypoints_.insert(waypoints_.begin() + insertAt, hit);
				selectedIndex_ = static_cast<int>(insertAt);
				dragIndex_ = selectedIndex_; // 置いたままドラッグで微調整できる
			}
		}
	}

	// ドラッグ中：水面上の位置に動かす
	if (dragIndex_ >= 0 && dragIndex_ < static_cast<int>(waypoints_.size())) {
		Vector3 hit;
		if (screenToWater(hit)) {
			waypoints_[dragIndex_] = hit;
		}
	}

	// 右クリック：点の上なら削除
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && hoverIndex_ >= 0) {
		waypoints_.erase(waypoints_.begin() + hoverIndex_);
		selectedIndex_ = -1;
		dragIndex_ = -1;
		hoverIndex_ = -1;
		targetIndex_ = 0;
	}
}

void BoatDriverComponent::DrawWaypointList() {
	int removeIndex = -1;
	int swapA = -1, swapB = -1;

	for (int i = 0; i < static_cast<int>(waypoints_.size()); ++i) {
		ImGui::PushID(i);

		// 番号（クリックで選択）
		char label[16];
		sprintf_s(label, "%02d", i);
		if (ImGui::Selectable(label, selectedIndex_ == i, 0, ImVec2(24.0f, 0.0f))) {
			selectedIndex_ = i;
		}
		ImGui::SameLine();

		// 座標（X, Z）
		float xz[2] = { waypoints_[i].x, waypoints_[i].z };
		ImGui::SetNextItemWidth(140.0f);
		if (ImGui::DragFloat2("##xz", xz, 0.1f)) {
			waypoints_[i].x = xz[0];
			waypoints_[i].z = xz[1];
		}
		ImGui::SameLine();

		// 並べ替え・削除
		if (ImGui::ArrowButton("##up", ImGuiDir_Up) && i > 0) { swapA = i; swapB = i - 1; }
		ImGui::SameLine();
		if (ImGui::ArrowButton("##down", ImGuiDir_Down) && i + 1 < static_cast<int>(waypoints_.size())) { swapA = i; swapB = i + 1; }
		ImGui::SameLine();
		if (ImGui::SmallButton("x")) { removeIndex = i; }

		ImGui::PopID();
	}

	// ループの外で配列を変更する
	if (swapA >= 0) {
		std::swap(waypoints_[swapA], waypoints_[swapB]);
		selectedIndex_ = swapB;
	}
	if (removeIndex >= 0) {
		waypoints_.erase(waypoints_.begin() + removeIndex);
		selectedIndex_ = -1;
		targetIndex_ = 0;
	}
}
#endif

void BoatDriverComponent::ImGui() {
#ifdef USEIMGUI
	ImGui::Text("--- Boat Driver ---");

	// 状態の確認
	if (ImGui::CollapsingHeader("Status", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("speed %.2f m/s (%.1f km/h)", speed_, speed_ * 3.6f);
		ImGui::Text("yaw %.2f rad  distance %.1f m", yaw_, distance_);
		if (waypoints_.empty()) {
			ImGui::Text("course: none (straight)");
		} else if (targetIndex_ < waypoints_.size()) {
			ImGui::Text("course: heading to %d / %d", static_cast<int>(targetIndex_), static_cast<int>(waypoints_.size()));
		} else {
			ImGui::Text("course: finished");
		}
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

	// コース
	if (ImGui::CollapsingHeader("Course (Waypoints)", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("Edit Waypoints", &isEditing_);
		if (isEditing_) {
			ImGui::TextDisabled("Game view: L-click add / drag move");
			ImGui::TextDisabled("Shift+L-click insert / R-click delete");
			HandleWaypointEditing();
		} else {
			hoverIndex_ = -1;
			dragIndex_ = -1;
		}

		ImGui::Checkbox("Loop", &loop_);
		ImGui::SameLine();
		ImGui::Checkbox("Show Preview", &showPreview_);

		// 旋回速度は度で表示すると分かりやすい
		float turnDeg = maxTurnRate_ * 180.0f / kPi;
		if (ImGui::DragFloat("Max Turn [deg/s]", &turnDeg, 0.5f, 1.0f, 180.0f)) {
			maxTurnRate_ = turnDeg * kPi / 180.0f;
		}
		if (maxTurnRate_ > 0.0f) {
			ImGui::Text("turn radius at cruise: %.1f m", cruiseSpeed_ / maxTurnRate_);
		}
		ImGui::DragFloat("Arrive Radius [m]", &arriveRadius_, 0.5f, 1.0f, 100.0f);

		ImGui::Text("points: %d", static_cast<int>(waypoints_.size()));
		ImGui::SameLine();
		if (ImGui::SmallButton("Reverse")) {
			std::reverse(waypoints_.begin(), waypoints_.end());
			selectedIndex_ = -1;
			targetIndex_ = 0;
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("Clear")) {
			waypoints_.clear();
			selectedIndex_ = -1;
			targetIndex_ = 0;
		}

		DrawWaypointList();
	}
#endif
}

void BoatDriverComponent::Serialize(json& j) const {
	j["type"] = "BoatDriverComponent";
	j["cruiseSpeed"] = cruiseSpeed_;
	j["acceleration"] = acceleration_;
	j["boatHeight"] = boatHeight_;
	j["isRunning"] = isRunning_;

	// コース（点は [x, z] の配列で保存）
	json points = json::array();
	for (const auto& p : waypoints_) {
		points.push_back({ p.x, p.z });
	}
	j["waypoints"] = points;
	j["loop"] = loop_;
	j["maxTurnRate"] = maxTurnRate_;
	j["arriveRadius"] = arriveRadius_;
	j["showPreview"] = showPreview_;
}

void BoatDriverComponent::Deserialize(const json& j) {
	if (j.contains("cruiseSpeed")) cruiseSpeed_ = j["cruiseSpeed"];
	if (j.contains("acceleration")) acceleration_ = j["acceleration"];
	if (j.contains("boatHeight")) boatHeight_ = j["boatHeight"];
	if (j.contains("isRunning")) isRunning_ = j["isRunning"];

	// コース
	if (j.contains("waypoints") && j["waypoints"].is_array()) {
		waypoints_.clear();
		for (const auto& p : j["waypoints"]) {
			if (p.is_array() && p.size() >= 2) {
				waypoints_.push_back({ p[0].get<float>(), 0.0f, p[1].get<float>() });
			}
		}
	}
	if (j.contains("loop")) loop_ = j["loop"];
	if (j.contains("maxTurnRate")) maxTurnRate_ = j["maxTurnRate"];
	if (j.contains("arriveRadius")) arriveRadius_ = j["arriveRadius"];
	if (j.contains("showPreview")) showPreview_ = j["showPreview"];
}
