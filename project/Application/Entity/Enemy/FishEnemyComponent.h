#pragma once
#include "Component.h"

enum class FishState {
	Submerge, // 潜水
	Jump      // ジャンプ
};

class FishEnemyComponent : public Component {
public:
	FishEnemyComponent() = default;
	~FishEnemyComponent() override = default;

	void Initialize() override;
	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "FishEnemyComponent"; }

public:
	// 被弾処理・ステータス
	void TakeDamage(int damage);
	void OnDead();
	bool IsDead() const { return isDead_; }

	int GetHp() const { return hp_; }
	int GetMaxHp() const { return maxHp_; }
	void SetHp(int hp) { hp_ = hp; }
	void SetMaxHp(int maxHp) { maxHp_ = maxHp; }

	int GetEnergyReward() const { return energyReward_; }
	void SetEnergyReward(int reward) { energyReward_ = reward; }

private:
	int hp_ = 3;                  // 体力（弾3発で撃破）
	int maxHp_ = 3;               // 最大体力
	int energyReward_ = 15;       // 撃破時にプレイヤーに還元される電力
	float invincibilityTimer_ = 0.0f;     // 被弾後の無敵・点滅タイマー
	float invincibilityDuration_ = 0.15f; // 無敵時間

	Vector3 startPos_{};      // 往復の開始地点
	float moveRange_ = 10.0f; // 往復する範囲
	float speed_ = 5.0f;      // 移動スピード
	float direction_ = 1.0f;  // 現在の移動方向（1.0f: 右, -1.0f: 左）
	bool isInitialized_ = false; // リセット防止

	// 挙動用の追加メンバ
	FishState state_ = FishState::Submerge;
	float stateTimer_ = 0.0f;
	float swimPhase_ = 0.0f;        // 泳ぎ（S字蛇行）の位相角
	Vector3 velocity_{};

	float submergeDuration_ = 3.0f; // 潜水時間
	float jumpPowerY_ = 15.0f;      // ジャンプ初速Y
	float jumpPowerXZ_ = 10.0f;     // ジャンプ初速XZ
	float gravity_ = -25.0f;        // 重力
	float waterSurfaceY_ = 0.0f;    // 水面の高さ
	float rotLerpSpeed_ = 5.0f;     // 回転補間の速度

	bool isDead_ = false;       // 死亡フラグ
	float deathTimer_ = 0.0f;   // 死亡演出タイマー
	Vector3 originalScale_{};   // 死亡時の初期スケール
};