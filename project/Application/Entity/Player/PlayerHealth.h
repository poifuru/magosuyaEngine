#pragma once
#include "MathTypes.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class GameObject;

class PlayerHealth {
public:
	PlayerHealth() = default;
	~PlayerHealth() = default;

	void Initialize();
	void Update(GameObject* gameObject);
	void TakeDamage(GameObject* gameObject, int damage);

	void ImGui();
	void Serialize(json& j) const;
	void Deserialize(const json& j);

	// ゲッター
	int GetHp() const { return hp_; }
	int GetMaxHp() const { return maxHp_; }
	bool IsDead() const { return isDead_; }

private:
	int hp_ = 5;
	int maxHp_ = 5;
	float invincibilityTimer_ = 0.0f;
	float invincibilityDuration_ = 1.5f;
	bool isDead_ = false;
};
