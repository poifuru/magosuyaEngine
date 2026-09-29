#pragma once
#include "Component.h"
#include "MathTypes.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class BossComponent : public Component {
public:
	BossComponent() = default;
	~BossComponent() override = default;

	void Initialize() override;
	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "BossComponent"; }

	// 被弾処理・死亡判定
	void TakeDamage(int damage);
	void OnDead();

	// ゲッター・セッター
	int GetHp() const { return hp_; }
	int GetMaxHp() const { return maxHp_; }
	bool IsDead() const { return isDead_; }

	void SetHp(int hp) { hp_ = hp; }
	void SetMaxHp(int maxHp) { maxHp_ = maxHp; }

private:
	// ステータス
	int hp_ = 50;
	int maxHp_ = 50;
	bool isDead_ = false;

	float invincibilityTimer_ = 0.0f;
	float invincibilityDuration_ = 0.15f; // 連続ヒット時の点滅タイマー

	// 深海での遊泳パラメータ
	float swimTime_ = 0.0f;
	float moveSpeed_ = 4.5f;             // 遊泳スピード
	float turnSpeed_ = 0.8f;             // 旋回スピード
	float bobbingSpeed_ = 1.2f;          // 上下のうねり周波数
	float bobbingAmount_ = 2.0f;         // 上下のうねり幅
	float targetDistance_ = 35.0f;       // プレイヤーとの間合い

	// 死亡時アニメーション
	float deathTimer_ = 0.0f;
};

// 互換性用エイリアス
using Boss = BossComponent;