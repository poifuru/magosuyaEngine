#pragma once
#include "Component.h"
#include "MathTypes.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class FloatingCrateComponent : public Component {
public:
	FloatingCrateComponent() = default;
	~FloatingCrateComponent() override = default;

	void Initialize() override;
	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "FloatingCrateComponent"; }

	// 被弾・破壊
	void TakeDamage(int damage);
	void OnDestroyed();
	bool IsDestroyed() const { return isDestroyed_; }

	int GetHp() const { return hp_; }
	void SetHp(int hp) { hp_ = hp; }

private:
	int hp_ = 1;                     // 耐久力（1発で壊れる）
	bool isDestroyed_ = false;

	float waterSurfaceY_ = 0.0f;     // 水面の高さ
	float bobbingSpeed_ = 2.0f;      // 上下揺れの速さ
	float bobbingAmount_ = 0.25f;    // 上下揺れの振幅
	float bobbingTimer_ = 0.0f;      // 揺れタイマー
	Vector3 driftVelocity_ = { 0.3f, 0.0f, 0.15f }; // ゆっくり漂流する速度

	float lifetime_ = 120.0f;        // 生存時間（秒）
	float lifeTimer_ = 0.0f;
};
