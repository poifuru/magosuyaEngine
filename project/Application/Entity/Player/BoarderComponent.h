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
//   この初速で届く高さ（そのままの高さ）が softKneeHeight_ を超えたら、超えた分は maxJumpHeight_ に向かって
//   だんだん伸びにくくする（ソフトな上限）。決めた高さ H から初速 √(2gH) を出す
//   → 大きい波ほど少しずつ高くなり、上限に張り付いて全部同じ高さ、にならない
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
	float GetCurrentJumpHeight() const { return currentJumpHeight_; } // 今のジャンプでここまでの最高点（水面から）[m]
	float GetLastJumpHeight() const { return lastJumpHeight_; } // 直前のジャンプの最高到達点（水面から）[m]
	float GetBestJumpHeight() const { return bestJumpHeight_; } // これまでの最高記録[m]
	float GetRopeLength() const { return ropeLength_; }   // ロープの長さ[m]（イルカの並走位置の計算に使う）
	float GetSternOffset() const { return sternOffset_; } // 船の中心からロープの付け根まで[m]
	float GetLastTakeoffRise() const { return lastTakeoffRise_; } // 直前の踏み切りの riseMemory[m/s]（演出の判定などに使う）

	// riseMemory から跳ぶ高さ[m]を決める（rawHeight にはソフトな上限をかける前の高さを入れる）
	float CalcJumpHeight(float rise, float* rawHeight = nullptr) const;

	// 入力の受付を外から切り替える（カウントダウン中・ゲーム終了後に止める）
	void SetSwingEnabled(bool enabled) { swingEnabled_ = enabled; } // 横移動
	void SetJumpEnabled(bool enabled) { jumpEnabled_ = enabled; }   // ジャンプ

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
	float maxJumpHeight_ = 40.0f;   // 最高到達点[m]（ソフトな上限が近づいていく先。ここには届かない）
	float softKneeHeight_ = 15.0f;  // ここまではそのままの高さ。超えた分は伸びにくくなる[m]
	float gravity_ = 25.0f;         // 重力[m/s²]（高さは変えず、滞空時間を変える）
	float riseMemoryDecay_ = 5.0f;  // 踏み切りの猶予が減る速さ[(m/s)/s]
	float maxRiseMemory_ = 30.0f;   // 猶予として覚えておく値の上限[m/s]（安全装置。高さの上限はソフトな上限で決める）

	// --- 波を立てる ---
	float rideWaveStrength_ = 0.3f;  // 滑っている間に立てる波の強さ（船 = 1）
	float rideWaveRadius_ = 1.5f;    // 滑っている間の波の半径[m]
	float rideFoam_ = 0.02f;         // 滑っている間の泡（毎フレーム）
	float landingDepthScale_ = 0.06f; // 着水の押し込み深さ = 落ちる速さ × これ [m/(m/s)]
	float maxLandingDepth_ = 1.2f;    // 着水の押し込み深さの上限[m]
	float landingRadius_ = 2.5f;      // 着水の波の半径[m]
	float landingFoam_ = 0.8f;        // 着水の泡

	// --- 入力 ---
	bool useRightStick_ = true;     // テスト用：船がまだ左スティックで動いているあいだは右スティック

	// ===== 実行時の状態 =====

	// --- 船 ---
	Vector2 boatDir_ = { 0.0f, 1.0f }; // 船の進行方向（XZ、止まっているときは前回の値）
	Vector2 boatVel_{};                // 船の速度[m/s]
	Vector2 boatXZ_{};                 // 船の位置（XZ）
	Vector3 prevBoatPos_{};
	bool hasPrevBoatPos_ = false;
	static constexpr float kTeleportDistance = 5.0f; // 1フレームでこれ以上動いたらワープとみなす[m]

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

	// --- 入力の受付（GameFlowComponent が切り替える。シリアライズしない） ---
	bool swingEnabled_ = true;
	bool jumpEnabled_ = true;

	// --- ジャンプ ---
	State state_ = State::Riding;
	bool wasJumpHeld_ = false;
	float riseMemory_ = 0.0f;       // 直近の surfaceRise の最大値（タイミングの猶予）
	float vy_ = 0.0f;               // 縦の速度[m/s]
	float takeoffWaterY_ = 0.0f;    // 踏み切った位置の水面の高さ
	float currentJumpHeight_ = 0.0f;
	float lastJumpHeight_ = 0.0f;
	float bestJumpHeight_ = 0.0f;
	float lastLandingDepth_ = 0.0f; // 直前の着水で押し込んだ深さ（デバッグ表示）
	float lastTakeoffRise_ = 0.0f;  // 直前の踏み切りの riseMemory

	// --- 調整用の記録（普通の航跡とイルカの波で、踏み切りの値と高さを比べる） ---
	struct JumpLog {
		float rise = 0.0f;          // 踏み切りの riseMemory[m/s]
		float rawHeight = 0.0f;     // ソフトな上限をかける前の高さ[m]
		float targetHeight = 0.0f;  // 決めた高さ[m]
		float actualHeight = -1.0f; // 実際に届いた高さ[m]（着水で入る。-1 = 空中）
	};
	static constexpr int kJumpLogSize = 10;
	JumpLog jumpLog_[kJumpLogSize]{};
	int jumpLogCount_ = 0;          // これまでの記録の数（リングバッファの書き込み位置にも使う）

	// --- デバッグ表示 ---
	static constexpr int kHistorySize = 120;
	float riseHistory_[kHistorySize]{};
	int historyIndex_ = 0;
};
