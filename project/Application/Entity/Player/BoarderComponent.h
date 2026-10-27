#pragma once
#include "Component.h"
#include "MathTypes.h"
#include "WaterQuery.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ボートにロープで引かれて水面を滑るボーダー
// ・スティックで横向きの力をかけて、ロープの振り出し角を変える（水上でも慣性が効く）
// ・高さは水面（Gerstner + 波紋）に乗る
// ・ジャンプボタンを押している間ためて、離すとジャンプ
//   打ち上げ速度 = 足元の水面が持ち上がる速さ dot(∇h, 船に対する相対速度) × 誇張倍率 + 基本ジャンプ
//   （上限は最高到達点 maxJumpHeight_ から √(2gH) で決める）
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
	float GetBestJumpHeight() const { return bestJumpHeight_; } // これまでの最高記録[m]

private:
	// 船の位置・進行方向・速度を更新
	void UpdateBoat(float dt, const Vector3& boatPos, float boatYaw);
	// ロープに沿って水平位置を決める
	void UpdateRope(float dt);
	// 水面に乗る・ジャンプ・着地
	void UpdateBoard(float dt);

	float ReadSwingInput() const; // -1 ～ 1
	bool IsJumpHeld() const;

private:
	// ===== 調整パラメータ（シリアライズ対象） =====

	// --- ロープ ---
	float ropeLength_ = 30.0f;   // ロープの長さ[m]
	float sternOffset_ = 2.0f;   // 船の中心から船尾（ロープの付け根）までの距離[m]
	float maxAngle_ = 0.84f;     // 振り出し角の限界[rad]（ここでロープがピンと張る）

	// --- 横移動（振り出し角にかかる力） ---
	float edgeAccel_ = 3.0f;     // スティックで横に切る力[rad/s²]
	float ropePull_ = 4.0f;      // ロープが真後ろへ戻そうとする力
	float waterDrag_ = 1.5f;     // 水の抵抗（水上だけ）
	float airControl_ = 0.1f;    // 空中での操作の効き（0 = 慣性だけ、1 = 水上と同じ）

	// --- ジャンプ ---
	float launchScale_ = 4.0f;      // 水面の持ち上がる速さ → 打ち上げ速度の誇張倍率
	float baseJump_ = 1.0f;         // ボタンだけで出る小ジャンプ[m/s]
	float maxJumpHeight_ = 10.0f;   // 最高到達点[m]
	float gravity_ = 25.0f;         // 重力[m/s²]（高さは変えず、滞空時間を変える）
	float riseMemoryDecay_ = 5.0f;  // 踏み切りの猶予が減る速さ[(m/s)/s]
	float maxRiseMemory_ = 8.0f;    // 猶予として覚えておく値の上限[m/s]（安全装置）

	// --- 入力 ---
	bool useRightStick_ = true;     // テスト用：船がまだ左スティックで動いているあいだは右スティック

	// ===== 実行時の状態 =====

	// --- 船 ---
	Vector2 boatDir_ = { 0.0f, 1.0f }; // 船の進行方向（XZ、止まっているときは前回の値）
	Vector2 boatVel_{};                // 船の速度[m/s]
	Vector2 boatXZ_{};                 // 船の位置（XZ）
	Vector3 prevBoatPos_{};
	bool hasPrevBoatPos_ = false;

	// --- ロープ ---
	float angle_ = 0.0f;     // 振り出し角[rad]
	float angleVel_ = 0.0f;  // 振り出し角の角速度[rad/s]

	// --- 水面 ---
	static constexpr uint32_t kQuerySlot = WaterQuery::kBoarderSlot; // WaterQuery の枠（最後の枠をボーダー専用にする）
	Vector2 prevXZ_{};
	bool hasPrevXZ_ = false;
	Vector2 horizVel_{};       // 水平速度[m/s]
	float waterY_ = 0.0f;      // 足元の水面の高さ
	float surfaceRise_ = 0.0f; // 足元の水面が持ち上がる速さ（船に対する相対速度で計算）[m/s]

	// --- ジャンプ ---
	State state_ = State::Riding;
	bool wasJumpHeld_ = false;
	float riseMemory_ = 0.0f;       // 直近の surfaceRise の最大値（タイミングの猶予）
	float vy_ = 0.0f;               // 縦の速度[m/s]
	float takeoffWaterY_ = 0.0f;    // 踏み切った位置の水面の高さ
	float currentJumpHeight_ = 0.0f;
	float lastJumpHeight_ = 0.0f;
	float bestJumpHeight_ = 0.0f;

	// --- デバッグ表示 ---
	static constexpr int kHistorySize = 120;
	float riseHistory_[kHistorySize]{};
	int historyIndex_ = 0;
};
