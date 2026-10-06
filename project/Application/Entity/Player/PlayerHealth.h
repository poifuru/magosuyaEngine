#pragma once
#include "MathTypes.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class GameObject;

class PlayerHealth {
public:
	PlayerHealth() = default;
	~PlayerHealth();

	void Initialize();
	void Update(GameObject* gameObject);
	void TakeDamage(GameObject* gameObject, int damage);

	// 電力消費・回復
	bool ConsumeHealth(int amount);
	void Heal(int amount);

	void ImGui();
	void Serialize(json& j) const;
	void Deserialize(const json& j);

	// ゲッター・セッター
	int GetHp() const { return hp_; }
	int GetMaxHp() const { return maxHp_; }
	bool IsDead() const { return isDead_; }

	void SetEnableDrain(bool enable) { enableDrain_ = enable; }
	bool IsDrainEnabled() const { return enableDrain_; }
	void SetDrainInterval(float interval) { drainInterval_ = interval; }
	float GetDrainInterval() const { return drainInterval_; }

private:
	void CreateUI(GameObject* gameObject);
	void UpdateUI();

private:
	int hp_ = 100;
	int maxHp_ = 100;
	float invincibilityTimer_ = 0.0f;
	float invincibilityDuration_ = 1.5f;
	bool isDead_ = false;

	// 自然消費（潜水艦の維持電力）
	bool enableDrain_ = true;
	float drainInterval_ = 3.0f; // 何秒に1電力を消費するか（デフォルト3秒）
	float drainTimer_ = 0.0f;

	// UI用オブジェクト（外枠・背景・HPバー本体）
	GameObject* hpBorderObj_ = nullptr;
	GameObject* hpBgObj_ = nullptr;
	GameObject* hpBarObj_ = nullptr;
	bool uiCreated_ = false;
};
