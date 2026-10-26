#include "PCH.h"
#include "CourseEditorWindow.h"
#include "EditorManager.h"
#include "BaseScene.h"
#include "GameObject.h"
#include "BoatDriverComponent.h"
#include <algorithm>
#include <cmath>

namespace {
	// 色（ImGui の DrawList 用）
	const ImU32 kBgColor = IM_COL32(24, 34, 46, 255);         // 背景（海）
	const ImU32 kGridColor = IM_COL32(255, 255, 255, 18);     // 細かいグリッド
	const ImU32 kGridMajorColor = IM_COL32(255, 255, 255, 40); // 太いグリッド
	const ImU32 kAxisXColor = IM_COL32(230, 80, 80, 120);     // X 軸（z = 0 の線）
	const ImU32 kAxisZColor = IM_COL32(80, 120, 230, 120);    // Z 軸（x = 0 の線）
	const ImU32 kCourseColor = IM_COL32(255, 115, 25, 220);   // 点をつなぐ線
	const ImU32 kReturnColor = IM_COL32(255, 115, 25, 90);    // ループの戻り線
	const ImU32 kRadiusColor = IM_COL32(255, 115, 25, 50);    // 到着判定の円
	const ImU32 kPointColor = IM_COL32(255, 115, 25, 255);    // 点
	const ImU32 kFirstColor = IM_COL32(80, 255, 80, 255);     // 最初の点
	const ImU32 kSelectColor = IM_COL32(255, 255, 255, 255);  // 選択中の点
	const ImU32 kPreviewColor = IM_COL32(100, 255, 150, 230); // 実際に走る線
	const ImU32 kStartColor = IM_COL32(255, 215, 25, 255);    // スタート地点
	const ImU32 kBoatColor = IM_COL32(50, 230, 255, 255);     // ボート
	const ImU32 kTargetColor = IM_COL32(255, 50, 50, 255);    // 今向かっている点
	const ImU32 kGoalColor = IM_COL32(255, 255, 255, 255);    // ゴール（白）
	const ImU32 kGoalSubColor = IM_COL32(20, 20, 20, 255);    // ゴール（チェッカーの黒）
	const ImU32 kTextColor = IM_COL32(255, 255, 255, 220);

	constexpr float kPointRadiusPx = 6.0f; // 点の見た目の半径（ピクセル）
	constexpr float kPickRadiusPx = 10.0f; // 点を掴める距離（ピクセル）

	// ズームに合わせて、グリッドの間隔を 1, 2, 5, 10, 20, 50... m から選ぶ
	float ChooseGridStep(float pixelsPerMeter) {
		const float kTargetPx = 40.0f; // 画面上でだいたいこのくらいの間隔にしたい
		float meters = kTargetPx / pixelsPerMeter;
		float base = std::pow(10.0f, std::floor(std::log10(meters)));
		if (meters <= base * 1.0f) return base;
		if (meters <= base * 2.0f) return base * 2.0f;
		if (meters <= base * 5.0f) return base * 5.0f;
		return base * 10.0f;
	}
}

CourseEditorWindow::CourseEditorWindow()
	: IEditorWindow("コースエディタ", false) {
}

BoatDriverComponent* CourseEditorWindow::FindTarget(const EditorContext& context) const {
	// 選択中のオブジェクトが持っていればそれ
	if (context.selectedObject && *context.selectedObject) {
		if (auto* driver = (*context.selectedObject)->GetComponent<BoatDriverComponent>()) {
			return driver;
		}
	}
	// 無ければシーン内の最初のもの
	if (context.sceneContext && context.sceneContext->activeGameObjects) {
		for (auto& obj : *context.sceneContext->activeGameObjects) {
			if (auto* driver = obj->GetComponent<BoatDriverComponent>()) {
				return driver;
			}
		}
	}
	return nullptr;
}

ImVec2 CourseEditorWindow::WorldToMap(float x, float z) const {
	ImVec2 center = { canvasPos_.x + canvasSize_.x * 0.5f, canvasPos_.y + canvasSize_.y * 0.5f };
	// 画面の上を +Z にするので、z は符号を反転
	return { center.x + (x - viewCenterX_) * pixelsPerMeter_, center.y - (z - viewCenterZ_) * pixelsPerMeter_ };
}

Vector3 CourseEditorWindow::MapToWorld(const ImVec2& p) const {
	ImVec2 center = { canvasPos_.x + canvasSize_.x * 0.5f, canvasPos_.y + canvasSize_.y * 0.5f };
	return { viewCenterX_ + (p.x - center.x) / pixelsPerMeter_, 0.0f, viewCenterZ_ - (p.y - center.y) / pixelsPerMeter_ };
}

void CourseEditorWindow::FitView(const BoatDriverComponent* driver, const ImVec2& canvasSize) {
	if (!driver || canvasSize.x <= 1.0f || canvasSize.y <= 1.0f) return;

	// スタート地点と全ての点が入る範囲
	float minX = driver->startPos_.x, maxX = minX;
	float minZ = driver->startPos_.z, maxZ = minZ;
	for (const auto& p : driver->waypoints_) {
		minX = std::min(minX, p.pos.x); maxX = std::max(maxX, p.pos.x);
		minZ = std::min(minZ, p.pos.z); maxZ = std::max(maxZ, p.pos.z);
	}
	// 到着判定の円ぶん＋余白
	float margin = driver->arriveRadius_ + 20.0f;
	minX -= margin; maxX += margin;
	minZ -= margin; maxZ += margin;

	viewCenterX_ = (minX + maxX) * 0.5f;
	viewCenterZ_ = (minZ + maxZ) * 0.5f;
	float sx = canvasSize.x / (maxX - minX);
	float sz = canvasSize.y / (maxZ - minZ);
	pixelsPerMeter_ = std::clamp(std::min(sx, sz), 0.02f, 50.0f);
}

void CourseEditorWindow::UpdateAndDraw(const EditorContext& context) {
#ifdef USEIMGUI
	if (!isOpen_) return;

	ImGui::SetNextWindowSize(ImVec2(900.0f, 560.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin(name_.c_str(), &isOpen_)) {
		BoatDriverComponent* driver = FindTarget(context);
		if (!driver) {
			ImGui::TextDisabled("BoatDriverComponent を持つオブジェクトがシーンにありません");
		} else {
			// 左：設定パネル（幅固定）
			ImGui::BeginChild("CourseSide", ImVec2(300.0f, 0.0f), true);
			DrawSidePanel(driver);
			ImGui::EndChild();

			ImGui::SameLine();

			// 右：マップ
			ImGui::BeginChild("CourseMap", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			DrawMap(driver);
			ImGui::EndChild();

			// ゲーム画面（3D）での編集
			if (driver->isEditing_) {
				driver->HandleWaypointEditing();
			}
		}
	}
	ImGui::End();
#endif
}

void CourseEditorWindow::DrawSidePanel(BoatDriverComponent* driver) {
#ifdef USEIMGUI
	ImGui::Text("Target: %s", driver->GetGameObject() ? driver->GetGameObject()->GetName().c_str() : "-");
	ImGui::Separator();

	// --- コースの設定 ---
	ImGui::Checkbox("Loop", &driver->loop_);
	ImGui::SameLine();
	if (ImGui::Checkbox("Preview", &showPreview_)) {
		driver->showPreview_ = showPreview_; // 3D の表示もそろえる
	}
	ImGui::SameLine();
	ImGui::Checkbox("Radius", &showRadius_);

	ImGui::SetNextItemWidth(140.0f);
	ImGui::DragFloat("Cruise [m/s]", &driver->cruiseSpeed_, 0.1f, 0.0f, 40.0f);

	// 旋回速度は度で表示すると分かりやすい
	const float kRadToDeg = 180.0f / 3.14159265f;
	float turnDeg = driver->maxTurnRate_ * kRadToDeg;
	ImGui::SetNextItemWidth(140.0f);
	if (ImGui::DragFloat("Max Turn [deg/s]", &turnDeg, 0.5f, 1.0f, 180.0f)) {
		driver->maxTurnRate_ = turnDeg / kRadToDeg;
	}
	if (driver->maxTurnRate_ > 0.0f) {
		ImGui::TextDisabled("turn radius at cruise: %.1f m", driver->cruiseSpeed_ / driver->maxTurnRate_);
	}
	ImGui::SetNextItemWidth(140.0f);
	ImGui::DragFloat("Arrive Radius [m]", &driver->arriveRadius_, 0.5f, 1.0f, 100.0f);

	// コースの長さ（点をつないだ長さ）と、1周の時間の目安
	float length = 0.0f;
	const auto& wps = driver->waypoints_;
	for (size_t i = 0; i + 1 < wps.size(); ++i) {
		length += std::hypot(wps[i + 1].pos.x - wps[i].pos.x, wps[i + 1].pos.z - wps[i].pos.z);
		if (wps[i].type == BoatDriverComponent::WaypointType::Goal) break; // ゴールの先は走らない
	}
	if (driver->loop_ && wps.size() >= 2 && !driver->HasGoal()) {
		length += std::hypot(wps.front().pos.x - wps.back().pos.x, wps.front().pos.z - wps.back().pos.z);
	}
	ImGui::TextDisabled("course length: %.0f m", length);
	if (driver->cruiseSpeed_ > 0.0f) {
		ImGui::TextDisabled("lap time (approx): %.1f s", length / driver->cruiseSpeed_);
	}

	ImGui::Separator();

	// --- 表示と操作 ---
	if (ImGui::Button("Fit (F)")) { needsFit_ = true; }
	ImGui::SameLine();
	ImGui::Checkbox("Follow Boat", &followBoat_);
	ImGui::Checkbox("Edit in Game View (3D)", &driver->isEditing_);
	ImGui::SetNextItemWidth(120.0f);
	ImGui::DragFloat("Goal Gate Width", &driver->gateWidth_, 0.1f, 2.0f, 200.0f, "%.1f m");

	ImGui::Text("points: %d", static_cast<int>(wps.size()));
	ImGui::SameLine();
	if (ImGui::SmallButton("Reverse")) {
		std::reverse(driver->waypoints_.begin(), driver->waypoints_.end());
		driver->selectedIndex_ = -1;
		driver->targetIndex_ = 0;
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Clear")) {
		driver->waypoints_.clear();
		driver->selectedIndex_ = -1;
		driver->targetIndex_ = 0;
	}

	ImGui::Separator();

	// --- 点の一覧 ---
	ImGui::BeginChild("CourseList", ImVec2(0.0f, 0.0f), false);
	driver->DrawWaypointList();
	ImGui::EndChild();
#endif
}

void CourseEditorWindow::DrawMap(BoatDriverComponent* driver) {
#ifdef USEIMGUI
	canvasPos_ = ImGui::GetCursorScreenPos();
	canvasSize_ = ImGui::GetContentRegionAvail();
	if (canvasSize_.x < 50.0f || canvasSize_.y < 50.0f) return;

	// マップ全体を1つのボタンにして、マウス入力を受け取る
	ImGui::InvisibleButton("CourseCanvas", canvasSize_,
		ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
	const bool isHovered = ImGui::IsItemHovered();
	const bool isActive = ImGui::IsItemActive();
	ImGuiIO& io = ImGui::GetIO();
	const ImVec2 mouse = io.MousePos;

	// --- 表示範囲の操作 ---
	if (needsFit_ || (isHovered && ImGui::IsKeyPressed(ImGuiKey_F))) {
		FitView(driver, canvasSize_);
		needsFit_ = false;
	}
	if (followBoat_ && driver->GetGameObject()) {
		const auto& t = driver->GetGameObject()->GetTransform();
		viewCenterX_ = t.translate.x;
		viewCenterZ_ = t.translate.z;
	}
	// ホイール：マウスの位置を中心にズーム
	if (isHovered && io.MouseWheel != 0.0f) {
		Vector3 before = MapToWorld(mouse);
		pixelsPerMeter_ = std::clamp(pixelsPerMeter_ * std::pow(1.15f, io.MouseWheel), 0.02f, 50.0f);
		Vector3 after = MapToWorld(mouse);
		viewCenterX_ += before.x - after.x;
		viewCenterZ_ += before.z - after.z;
	}
	// 中ドラッグ：スクロール
	if (isActive && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f)) {
		viewCenterX_ -= io.MouseDelta.x / pixelsPerMeter_;
		viewCenterZ_ += io.MouseDelta.y / pixelsPerMeter_;
	}

	auto& wps = driver->waypoints_;

	// --- マウスが乗っている点 ---
	int hover = -1;
	if (isHovered || isActive) {
		float best = kPickRadiusPx * kPickRadiusPx;
		for (int i = 0; i < static_cast<int>(wps.size()); ++i) {
			ImVec2 s = WorldToMap(wps[i].pos.x, wps[i].pos.z);
			float dx = s.x - mouse.x, dy = s.y - mouse.y;
			float d = dx * dx + dy * dy;
			if (d < best) { best = d; hover = i; }
		}
	}
	if (isHovered) {
		driver->hoverIndex_ = hover; // 3D 側でも白く光らせる
	}

	// --- 点の編集 ---
	// 左クリック：点の上なら掴む、何もなければ追加（Shift なら一番近い線の間に挿入）
	if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		if (hover >= 0) {
			driver->selectedIndex_ = hover;
			driver->dragIndex_ = hover;
		} else {
			driver->dragIndex_ = driver->AddWaypoint(MapToWorld(mouse), io.KeyShift);
		}
	}
	// ドラッグ中：マウスの位置に動かす
	if (driver->dragIndex_ >= 0 && driver->dragIndex_ < static_cast<int>(wps.size())) {
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			if (isActive) {
				Vector3 p = MapToWorld(mouse);
				wps[driver->dragIndex_].pos = { p.x, 0.0f, p.z };
			}
		} else {
			driver->dragIndex_ = -1;
		}
	}
	// 右クリック：点の上なら削除
	if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && hover >= 0) {
		driver->RemoveWaypoint(hover);
		hover = -1;
	}
	// Delete キー：選択中の点を削除
	if (isHovered && ImGui::IsKeyPressed(ImGuiKey_Delete) && driver->selectedIndex_ >= 0) {
		driver->RemoveWaypoint(driver->selectedIndex_);
	}
	// G キー：選択中の点を 道中 ⇔ ゴール に切り替える
	if (isHovered && ImGui::IsKeyPressed(ImGuiKey_G) &&
		driver->selectedIndex_ >= 0 && driver->selectedIndex_ < static_cast<int>(wps.size())) {
		auto& type = wps[driver->selectedIndex_].type;
		type = (type == BoatDriverComponent::WaypointType::Goal)
			? BoatDriverComponent::WaypointType::Pass : BoatDriverComponent::WaypointType::Goal;
		driver->targetIndex_ = 0;
	}

	// ===== 描画 =====
	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImVec2 p0 = canvasPos_;
	ImVec2 p1 = { canvasPos_.x + canvasSize_.x, canvasPos_.y + canvasSize_.y };
	dl->PushClipRect(p0, p1, true);
	dl->AddRectFilled(p0, p1, kBgColor);

	// --- グリッド ---
	float step = ChooseGridStep(pixelsPerMeter_);
	Vector3 wMin = MapToWorld({ p0.x, p1.y }); // 左下
	Vector3 wMax = MapToWorld({ p1.x, p0.y }); // 右上
	for (float x = std::floor(wMin.x / step) * step; x <= wMax.x; x += step) {
		bool major = std::fmod(std::abs(x) + 0.001f, step * 5.0f) < step * 0.5f;
		ImVec2 a = WorldToMap(x, wMin.z), b = WorldToMap(x, wMax.z);
		dl->AddLine(a, b, std::abs(x) < step * 0.5f ? kAxisZColor : (major ? kGridMajorColor : kGridColor));
	}
	for (float z = std::floor(wMin.z / step) * step; z <= wMax.z; z += step) {
		bool major = std::fmod(std::abs(z) + 0.001f, step * 5.0f) < step * 0.5f;
		ImVec2 a = WorldToMap(wMin.x, z), b = WorldToMap(wMax.x, z);
		dl->AddLine(a, b, std::abs(z) < step * 0.5f ? kAxisXColor : (major ? kGridMajorColor : kGridColor));
	}

	// --- 到着判定の円 ---
	if (showRadius_) {
		for (const auto& w : wps) {
			dl->AddCircle(WorldToMap(w.pos.x, w.pos.z), driver->arriveRadius_ * pixelsPerMeter_, kRadiusColor, 48);
		}
	}

	// --- 点をつなぐ線 ---
	for (size_t i = 0; i + 1 < wps.size(); ++i) {
		dl->AddLine(WorldToMap(wps[i].pos.x, wps[i].pos.z), WorldToMap(wps[i + 1].pos.x, wps[i + 1].pos.z), kCourseColor, 2.0f);
	}
	if (driver->loop_ && wps.size() >= 2 && !driver->HasGoal()) {
		dl->AddLine(WorldToMap(wps.back().pos.x, wps.back().pos.z), WorldToMap(wps.front().pos.x, wps.front().pos.z), kReturnColor, 2.0f);
	}

	// --- ゴールのゲート（白黒のチェッカー線） ---
	for (size_t i = 0; i < wps.size(); ++i) {
		if (wps[i].type != BoatDriverComponent::WaypointType::Goal) continue;
		Vector3 l, r;
		driver->GetGoalGate(i, l, r);
		const int kStripes = 8;
		for (int k = 0; k < kStripes; ++k) {
			float t0 = static_cast<float>(k) / kStripes, t1 = static_cast<float>(k + 1) / kStripes;
			ImVec2 a = WorldToMap(l.x + (r.x - l.x) * t0, l.z + (r.z - l.z) * t0);
			ImVec2 b = WorldToMap(l.x + (r.x - l.x) * t1, l.z + (r.z - l.z) * t1);
			dl->AddLine(a, b, (k % 2 == 0) ? kGoalColor : kGoalSubColor, 4.0f);
		}
		// ゲートの両端（ウキが置かれる位置）
		dl->AddCircleFilled(WorldToMap(l.x, l.z), 4.0f, IM_COL32(255, 140, 0, 255), 12);
		dl->AddCircleFilled(WorldToMap(r.x, r.z), 4.0f, IM_COL32(255, 140, 0, 255), 12);
	}

	// --- 実際に走る線 ---
	if (showPreview_ && !wps.empty()) {
		driver->BuildPreviewPath(previewPath_);
		for (size_t i = 1; i < previewPath_.size(); ++i) {
			dl->AddLine(WorldToMap(previewPath_[i - 1].x, previewPath_[i - 1].z), WorldToMap(previewPath_[i].x, previewPath_[i].z), kPreviewColor, 1.5f);
		}
	}

	// --- スタート地点（円と向き） ---
	{
		ImVec2 s = WorldToMap(driver->startPos_.x, driver->startPos_.z);
		float len = 24.0f;
		ImVec2 tip = { s.x + std::sin(driver->startYaw_) * len, s.y - std::cos(driver->startYaw_) * len };
		dl->AddCircle(s, 8.0f, kStartColor, 24, 2.0f);
		dl->AddLine(s, tip, kStartColor, 2.0f);
		dl->AddText({ s.x + 10.0f, s.y + 4.0f }, kStartColor, "START");
	}

	// --- 点 ---
	for (int i = 0; i < static_cast<int>(wps.size()); ++i) {
		ImVec2 s = WorldToMap(wps[i].pos.x, wps[i].pos.z);
		bool isGoal = wps[i].type == BoatDriverComponent::WaypointType::Goal;
		bool hot = (i == driver->selectedIndex_) || (i == hover);
		ImU32 col = hot ? kSelectColor : (isGoal ? kGoalColor : (i == 0 ? kFirstColor : kPointColor));
		dl->AddCircleFilled(s, hot ? kPointRadiusPx + 2.0f : kPointRadiusPx, col, 16);
		if (isGoal) {
			dl->AddCircle(s, kPointRadiusPx + 2.0f, kGoalSubColor, 16, 2.0f);
		}
		char label[16];
		sprintf_s(label, isGoal ? "%d GOAL" : "%d", i);
		dl->AddText({ s.x + 8.0f, s.y - 16.0f }, kTextColor, label);
	}

	// --- 今向かっている点 ---
	if (driver->targetIndex_ < wps.size()) {
		const auto& w = wps[driver->targetIndex_];
		dl->AddCircle(WorldToMap(w.pos.x, w.pos.z), kPointRadiusPx + 6.0f, kTargetColor, 24, 2.0f);
	}

	// --- ボート（進行方向を向いた三角形） ---
	if (driver->GetGameObject()) {
		const auto& t = driver->GetGameObject()->GetTransform();
		ImVec2 c = WorldToMap(t.translate.x, t.translate.z);
		float yaw = driver->yaw_;
		// 画面上の前方向（上が +Z なので y は反転）と右方向
		ImVec2 f = { std::sin(yaw), -std::cos(yaw) };
		ImVec2 r = { -f.y, f.x };
		float size = 9.0f;
		ImVec2 a = { c.x + f.x * size * 1.6f, c.y + f.y * size * 1.6f };
		ImVec2 b = { c.x - f.x * size + r.x * size, c.y - f.y * size + r.y * size };
		ImVec2 d = { c.x - f.x * size - r.x * size, c.y - f.y * size - r.y * size };
		dl->AddTriangleFilled(a, b, d, kBoatColor);
	}

	// --- 左上：縮尺と操作説明 ---
	char info[128];
	sprintf_s(info, "grid: %g m   zoom: %.2f px/m", step, pixelsPerMeter_);
	dl->AddText({ p0.x + 8.0f, p0.y + 6.0f }, kTextColor, info);
	dl->AddText({ p0.x + 8.0f, p0.y + 24.0f }, IM_COL32(255, 255, 255, 120),
		"L: add / drag   Shift+L: insert   R / Del: delete   G: goal   Wheel: zoom   M-drag: pan   F: fit");

	// マウス位置のワールド座標
	if (isHovered) {
		Vector3 w = MapToWorld(mouse);
		char pos[64];
		sprintf_s(pos, "x %.1f  z %.1f", w.x, w.z);
		dl->AddText({ p0.x + 8.0f, p1.y - 20.0f }, kTextColor, pos);
	}

	dl->PopClipRect();
#endif
}
