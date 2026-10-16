#pragma once
#include "Component.h"
#include <string>

enum class GamePhase {
	Surface,
	DiveChase,
	BossBattle
};

class GameDirectorComponent : public Component {
public:
	GameDirectorComponent() = default;
	~GameDirectorComponent() override = default;

	void Initialize() override;
	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "Game Director"; }

	// 敵死亡通知
	void NotifyEnemyDead();

	// 目標撃破数達成時のイベント
	void OnTargetKillsAchieved();

	// 各種アクセッサ
	void SetTargetKills(int target) { targetKills_ = target; }
	int GetTargetKills() const { return targetKills_; }

	void SetCurrentKills(int current) { currentKills_ = current; }
	int GetCurrentKills() const { return currentKills_; }

	void SetUIObjectName(const std::string& name) { uiObjectName_ = name; }
	const std::string& GetUIObjectName() const { return uiObjectName_; }

	GamePhase IsBossEventTriggered() const { return gamePhase_; }

private:
	// UIへの残り撃破数の反映
	void UpdateUI();

private:
	int targetKills_ = 10;           // 目標撃破数
	int currentKills_ = 0;           // 現在の撃破数
	GamePhase gamePhase_ = GamePhase::Surface;

	std::string uiObjectName_ = "KillCounterUI"; // UIオブジェクトの名前
};
