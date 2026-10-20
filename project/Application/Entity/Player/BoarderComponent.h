#pragma once
#include "Component.h"
#include "MathTypes.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ボートにロープで引かれて水面を滑るボーダー
// ・スティックでロープの振り出し角を操作して左右に振れる
// ・高さは水面（Gerstner + 波紋）に乗る
// ・ジャンプボタンを押している間ためて、離すとジャンプ
//   打ち上げ速度 = 足元の水面が持ち上がる速さ dot(∇h, v) × 誇張倍率 + 基本ジャンプ
class BoarderComponent : public Component {
public:
	enum class State {
		Riding,   // 水面を滑っている
		Airborne, // 空中
	};

	BoarderComponent() = default;
	~BoarderComponent() override = default;

	void Initialize() override;
	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "BoarderComponent"; }

	State GetState() const { return state_; }
	float GetLastJumpHeight() const { return lastJumpHeight_; } // 直前のジャンプの最高到達点（水面から）[m]

private:
	// ロープに沿って水平位置を決める
	void UpdateRope(float dt, const Vector3& boatPos, const Vector2& boatDir);
	// 水面に乗る・ジャンプ・着地
	void UpdateBoard(float dt);

	float ReadSwingInput() const; // -1 ～ 1
	bool IsJumpHeld() const;

private:
	// --- ロープ ---
	float ropeLength_ = 15.0f;      // ロープの長さ[m]
	float sternOffset_ = 2.0f;      // 船の中心から船尾（ロープの付け根）までの距離[m]
	float maxAngle_ = 0.7f;         // 最大振り出し角[rad]（約40度）
	float angleStiffness_ = 6.0f;   // 振り出し角が目標に追いつく速さ
	float angle_ = 0.0f;            // 今の振り出し角[rad]
	float angleVel_ = 0.0f;         // 振り出し角の角速度[rad/s]
	Vector2 boatDir_ = { 0.0f, 1.0f };     // 船の進行方向（XZ、止まっているときは前回の値）
	Vector3 prevBoatPos_{};
	bool hasPrevBoatPos_ = false;

	// テスト用：船がまだ左スティックで動いているあいだは右スティックで操作する
	bool useRightStick_ = true;

	// --- 水面 ---
	static constexpr uint32_t kQuerySlot = 15; // WaterQuery の枠（最後の枠をボーダー専用にする）
	Vector2 prevXZ_{};
	bool hasPrevXZ_ = false;
	Vector2 horizVel_{};            // 水平速度[m/s]
	float waterY_ = 0.0f;           // 足元の水面の高さ
	float surfaceRise_ = 0.0f;      // 足元の水面が持ち上がる速さ[m/s]

	// --- ジャンプ ---
	State state_ = State::Riding;
	bool wasJumpHeld_ = false;
	float riseMemory_ = 0.0f;       // 直近の surfaceRise の最大値（タイミングの猶予）
	float riseMemoryDecay_ = 5.0f;  // 猶予が減る速さ[(m/s)/s]
	float launchScale_ = 4.0f;      // 誇張倍率
	float baseJump_ = 1.0f;         // ボタンだけで出る小ジャンプ[m/s]
	float gravity_ = 25.0f;
	float vy_ = 0.0f;
	float takeoffWaterY_ = 0.0f;
	float currentJumpHeight_ = 0.0f;
	float lastJumpHeight_ = 0.0f;
	float bestJumpHeight_ = 0.0f;
	Vector2 boatVel_{};
	float maxJumpHeight_ = 10.0f; // 最高到達点[m]

	// --- デバッグ表示 ---
	static constexpr int kHistorySize = 120;
	float riseHistory_[kHistorySize]{};
	int historyIndex_ = 0;
};
