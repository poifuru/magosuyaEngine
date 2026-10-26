#pragma once
#include "IEditorWindow.h"
#include "MathTypes.h"
#include <vector>

class BoatDriverComponent;

// ボートのコース（ウェイポイント）を真上から見たマップで編集するウィンドウ
// ・左：設定と点の一覧、右：マップ（XZ平面、画面の上が +Z）
// ・マップ上：左クリックで追加／点をドラッグで移動／Shift+左クリックで線の間に挿入／右クリックで削除
//            ホイールでズーム、中ドラッグでスクロール、F キーで全体表示
// ・編集対象は「選択中のオブジェクトの BoatDriverComponent」。無ければシーン内の最初のもの
class CourseEditorWindow : public IEditorWindow {
public:
	CourseEditorWindow();
	~CourseEditorWindow() override = default;

	void UpdateAndDraw(const EditorContext& context) override;

private:
	// 編集対象を探す
	BoatDriverComponent* FindTarget(const EditorContext& context) const;

	// 左の設定パネル
	void DrawSidePanel(BoatDriverComponent* driver);

	// 右のマップ（描画と操作）
	void DrawMap(BoatDriverComponent* driver);

	// コース全体が入るように表示範囲を合わせる
	void FitView(const BoatDriverComponent* driver, const ImVec2& canvasSize);

	// ワールド(XZ) ⇔ マップ上の画面座標
	ImVec2 WorldToMap(float x, float z) const;
	Vector3 MapToWorld(const ImVec2& p) const;

private:
	// 表示範囲
	float viewCenterX_ = 0.0f;    // マップ中央のワールド X
	float viewCenterZ_ = 0.0f;    // マップ中央のワールド Z
	float pixelsPerMeter_ = 2.0f; // ズーム（1m が何ピクセルか）

	// マップの矩形（毎フレーム更新）
	ImVec2 canvasPos_ = { 0.0f, 0.0f };
	ImVec2 canvasSize_ = { 0.0f, 0.0f };

	// 表示の切り替え
	bool showRadius_ = true;   // 到着判定の円
	bool showPreview_ = true;  // 実際に走る線
	bool followBoat_ = false;  // ボートを中央に追いかける

	// 最初に開いたときに全体表示にする
	bool needsFit_ = true;

	// プレビューの線（毎フレーム作り直す）
	std::vector<Vector3> previewPath_;
};
