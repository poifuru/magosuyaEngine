#pragma once
#include "Component.h"
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

class BoatDriverComponent;
class WaterSurfaceComponent;
class GameFlowComponent;
class DolphinEventComponent;

// 印を付けたウェイポイントを船が通過すると、海が荒れる（大きなうねりが立つ）イベント
// ・場所はコースエディタで固定（W キー、コースに1つだけ）。1プレイに1回だけ起きる
// ・うねりは WaterSurfaceComponent の「荒れ用の波」（Gerstner）。強さ 0〜1 をこのコンポーネントが動かす
//   波紋（Ripple）は船の周り 150 m しか計算しないので、「この辺り一帯が荒れる」には式で全体に出せる Gerstner を使う
// ・うねりの向きは Warning のときの船の向き（強さ 0 のうちに決めないと波が瞬間移動する）
//   → 山がボーダーに来る間隔（出会い周期）は T = 波長 / (船の速さ − うねりの速さ) で一定になり、狙って跳べる
// ・Waiting → Warning（予告。強さ 0）→ Rising（0→1）→ Peak（ここが跳ぶ時間）→ Calming（1→0）→ Done
// ・イルカと重ならないように、イルカが並走中なら通過しても Warning を待つ（抽選の段階でも離してある）
// ・GameFlow が Playing の間だけ動く
class RoughSeaComponent : public Component {
public:
	enum class State {
		Waiting,  // 印の点を船が通過するのを待っている
		Warning,  // 予告（メッセージだけ。うねりはまだ立たない）
		Rising,   // うねりが大きくなっていく
		Peak,     // いちばん荒れている（ここが跳ぶ時間）
		Calming,  // 静まっていく
		Done,     // 終わった（このプレイではもう起きない）
	};

	// noticeText_ の初期値（ステージセレクトがシーンjsonを読むときにも使う）
	static constexpr const char* kDefaultNoticeText = "このコースには\n大きなうねりが来る場所がある";

	RoughSeaComponent() = default;
	~RoughSeaComponent() override = default;

	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "Rough Sea"; }

	// ===== HUD・ほかのイベント用 =====
	State GetState() const { return state_; }
	float GetStateTime() const { return stateTime_; }
	float GetWeight() const { return weight_; }
	// 強い時間（予告〜いちばん荒れている間）か。イルカはこの間は始まらない
	bool IsBusy() const { return state_ == State::Warning || state_ == State::Rising || state_ == State::Peak; }
	// 強い時間の長さ[s]（イルカの抽選で、どれだけ離すかの見積もりに使う）
	float GetBusyDuration() const { return warningTime_ + riseTime_ + peakTime_; }
	const std::string& GetNoticeText() const { return noticeText_; }   // ステージセレクトに出す予告
	const std::string& GetWarningText() const { return warningText_; } // 通過したときに出す

private:
	bool FindReferences();
	void ChangeState(State next);
	void StartEvent(float strength); // Warning を始める（うねりの向きをここで決める）
	void ApplyWeight(float weight);  // 水面に強さを送る

	// 印の点の先（その点と次の点）で曲がる量[rad]。大きいとうねりのリズムが変わる（ImGui の注意表示用）
	float CalcTurnAfter(int index) const;

private:
	// ===== 調整パラメータ（シリアライズ対象） =====
	float warningTime_ = 2.0f;  // 予告の長さ[s]
	float riseTime_ = 2.0f;     // 大きくなる時間[s]
	float peakTime_ = 8.0f;     // いちばん荒れている時間[s]
	float calmTime_ = 3.0f;     // 静まる時間[s]

	// --- メッセージ（文字は Resources/Font/charset.txt に入っていること） ---
	std::string noticeText_ = kDefaultNoticeText;
	std::string warningText_ = "うねりが来る！\n山のてっぺんで跳ぼう";

	// ===== 実行時の状態 =====
	State state_ = State::Waiting;
	float stateTime_ = 0.0f;
	float strength_ = 1.0f;      // 印の点の強さ（Waypoint::rough）
	float weight_ = 0.0f;        // 今の強さ（0〜1）
	float calmFrom_ = 0.0f;      // Calming を始めたときの強さ（途中で終わっても急に消えないように）
	size_t prevTarget_ = 0;      // 前フレームに船が向かっていた点
	bool hasPrevTarget_ = false;
	bool pendingStart_ = false;  // 通過したが、イルカの並走中なので待っている
	float pendingStrength_ = 1.0f;

	BoatDriverComponent* boat_ = nullptr;
	WaterSurfaceComponent* water_ = nullptr;
	GameFlowComponent* flow_ = nullptr;
	DolphinEventComponent* dolphin_ = nullptr;
};
