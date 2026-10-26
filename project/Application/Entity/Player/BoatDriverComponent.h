#pragma once
#include "Component.h"
#include "MathTypes.h"
#include <nlohmann/json.hpp>
#include <vector>

using json = nlohmann::json;

// 船を自動で走らせるコンポーネント
// ・スタートから acceleration_ で加速し、cruiseSpeed_ で一定速度になる
// ・ウェイポイントがあれば順番にたどる（最大旋回速度 maxTurnRate_ で曲がる）。無ければまっすぐ進む
// ・速度は m/s で持ち、dt を掛けて動かすので FPS に左右されない
// ・コースの編集は「コースエディタ」ウィンドウ（CourseEditorWindow）で行う
class BoatDriverComponent : public Component {
	// エディタウィンドウからコースのデータと編集処理を直接触れるようにする
	friend class CourseEditorWindow;

public:
	// ウェイポイントの種類
	enum class WaypointType : uint8_t {
		Pass, // 道中（通過したら次の点へ）
		Goal, // ゴール（ここに着いたらコース終了、船は止まる）
	};

	struct Waypoint {
		Vector3 pos;                          // 位置（y は使わない）
		WaypointType type = WaypointType::Pass;
	};

	BoatDriverComponent() = default;
	~BoatDriverComponent() override = default;

	void Initialize() override;
	void Update() override;
	void ImGui() override;
	void DrawDebug() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "BoatDriverComponent"; }

	float GetSpeed() const { return speed_; }   // 今の速度[m/s]
	float GetYaw() const { return yaw_; }       // 進行方向[rad]

	// スタート地点に戻して、速度0からやり直す
	void ResetToStart();

	// ゴールに着いたか（ゲーム側でリザルトを出すときなどに使う）
	bool IsFinished() const { return isFinished_; }

	// コースの情報（ウキの配置などに使う）
	const std::vector<Waypoint>& GetWaypoints() const { return waypoints_; }
	const Vector3& GetStartPos() const { return startPos_; }

	// その点に入ってくる向き（XZの単位ベクトル）。前の点（0番はスタート地点）から見た方向
	Vector3 GetIncomingDir(size_t index) const;

	// ゴールのゲート（コースに垂直に gateWidth_ だけ離れた左右の2点）
	void GetGoalGate(size_t index, Vector3& left, Vector3& right) const;

private:
	// 操舵を1ステップ進める（Update と走行プレビューで同じ計算を使う）
	// pos に一番近い目標点を見て、yaw を最大旋回速度で目標の方向へ回す。到着したら target を進める
	// 戻り値：コースを走り終えたら false（ゴールに着いた、またはループしないで最後の点を過ぎた）
	bool SteerStep(const Vector3& pos, float& yaw, size_t& target, float dt) const;

	// ゴールの点があるか
	bool HasGoal() const;

	// 実際に走る線をシミュレーションして、通る点を out に詰める（3D表示とエディタで共通）
	void BuildPreviewPath(std::vector<Vector3>& out) const;

#ifdef USEIMGUI
	// ゲーム画面でのマウス操作（追加・移動・挿入・削除）
	void HandleWaypointEditing();
	// 点の一覧（座標・並べ替え・削除）
	void DrawWaypointList();
	// 点の追加・挿入・削除（ゲーム画面とエディタで共通）
	int AddWaypoint(const Vector3& p, bool insertNearest);
	void RemoveWaypoint(int index);
#endif

private:
	// ===== 調整パラメータ（シリアライズ対象） =====
	float cruiseSpeed_ = 10.0f;  // 巡航速度[m/s]
	float acceleration_ = 3.0f;  // 加速[m/s²]（スタート時に徐々に速くなる）
	float boatHeight_ = 0.3f;    // 船の高さ（水面に置く y）
	bool isRunning_ = true;      // false で停止（エディタでの確認用）

	// --- コース ---
	std::vector<Waypoint> waypoints_; // コースの点
	bool loop_ = true;               // 最後の点の次は最初の点へ
	float maxTurnRate_ = 0.3f;       // 最大旋回速度[rad/s]（旋回半径 = 速度 / これ）
	float arriveRadius_ = 15.0f;     // この距離まで近づいたら次の点へ[m]
	bool showPreview_ = true;        // 実際に走る線を表示する
	float gateWidth_ = 24.0f;        // ゴールのゲートの幅（2つのウキの間隔）[m]

	// ===== 実行中の状態 =====
	float speed_ = 0.0f;         // 今の速度[m/s]
	float yaw_ = 0.0f;           // 進行方向[rad]
	float distance_ = 0.0f;      // スタートからの走行距離[m]
	size_t targetIndex_ = 0;     // 今向かっているウェイポイント（size 以上なら走り終えた）
	bool isFinished_ = false;    // ゴールに着いた（減速して止まる）

	// スタート地点（Initialize 時、またはエディタ停止中の位置と向き）
	Vector3 startPos_{};
	float startYaw_ = 0.0f;

	// ===== エディタ操作 =====
	bool isEditing_ = false;     // ゲーム画面（3D）でのクリック編集を受け付ける
	int hoverIndex_ = -1;        // マウスが乗っている点
	int dragIndex_ = -1;         // ドラッグ中の点
	int selectedIndex_ = -1;     // 選択中の点（一覧と画面で共通）
};
