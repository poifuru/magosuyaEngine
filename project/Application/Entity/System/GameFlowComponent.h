#pragma once
#include "Component.h"
#include "BoarderComponent.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <algorithm>

using json = nlohmann::json;

class BoatDriverComponent;

// インゲームの進行を管理する（表示はしない。HUD は Getter を読む）
// Ready（カウントダウン）→ Playing → Ending（GOAL!/FINISH! を見せる）→ Result（リトライ / タイトル）
// ・船が Goal のウェイポイントに着いたらゴール
// ・maxJumps_ 回跳んで、最後のジャンプが着水したら強制終了
class GameFlowComponent : public Component {
public:
	enum class Phase { Ready, Playing, Ending, Result };
	enum class EndReason { None, Goal, OutOfJumps };

	GameFlowComponent() = default;
	~GameFlowComponent() override = default;

	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "GameFlowComponent"; }

	// ===== HUD 用 =====
	Phase GetPhase() const { return phase_; }
	EndReason GetEndReason() const { return endReason_; }
	float GetPhaseTime() const { return phaseTime_; } // 今の Phase に入ってからの時間[s]
	float GetCountdownTime() const { return countdownTime_; }
	int GetMaxJumps() const { return maxJumps_; }
	int GetJumpsUsed() const { return jumpsUsed_; }
	int GetRemainingJumps() const { return (std::max)(maxJumps_ - jumpsUsed_, 0); }
	const std::vector<float>& GetJumpHeights() const { return jumpHeights_; } // 着水したジャンプの高さ[m]
	float GetBestHeight() const;      // このプレイでの最高[m]
	bool IsNewRecord() const { return isNewRecord_; } // このプレイで自己ベストを更新したか

private:
	bool FindReferences();      // 船とボーダーを探す（見つかるまで毎フレーム試す）
	void StartRace();           // 最初のフレームで1回だけ：船を止めて、操作を止める
	void TrackJumps();          // 踏み切り・着水を数える（どの Phase でも呼ぶ）
	void ChangePhase(Phase next);
	void EndRace(EndReason reason);

	void UpdateReady();
	void UpdatePlaying();
	void UpdateEnding();
	void UpdateResult();

private:
	// ===== 調整パラメータ（シリアライズ対象） =====
	int maxJumps_ = 3;              // このステージで跳べる回数（3〜5）
	float countdownTime_ = 3.0f;    // 3,2,1 の長さ[s]
	float endingTime_ = 2.0f;       // GOAL!/FINISH! を見せる時間[s]（着水してから数える）
	float resultInputDelay_ = 0.5f; // リザルトが出てから入力を受け付けるまで[s]（連打で即リトライしないように）

	// ===== 実行時の状態 =====
	Phase phase_ = Phase::Ready;
	EndReason endReason_ = EndReason::None;
	float phaseTime_ = 0.0f;
	bool isStarted_ = false;

	int jumpsUsed_ = 0;
	std::vector<float> jumpHeights_;
	float prevBest_ = 0.0f;     // スタート時点の自己ベスト（シーンをまたいで static に覚えておく値）
	bool isNewRecord_ = false;
	BoarderComponent::State prevBoarderState_ = BoarderComponent::State::Riding;

	BoarderComponent* boarder_ = nullptr;
	BoatDriverComponent* boat_ = nullptr;

	// 自己ベスト（リトライでシーンを作り直しても残るように static。アプリを閉じると消える）
	static float sessionBest_;
};
