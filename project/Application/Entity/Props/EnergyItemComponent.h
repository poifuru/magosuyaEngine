#pragma once
#include "Component.h"
#include "MathTypes.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class PlayerComponent;

class EnergyItemComponent : public Component {
public:
	EnergyItemComponent() = default;
	~EnergyItemComponent() override = default;

	void Initialize() override;
	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "EnergyItemComponent"; }

	// 取得処理
	void OnCollect(PlayerComponent* player);
	bool IsCollected() const { return isCollected_; }

	int GetHealAmount() const { return healAmount_; }
	void SetHealAmount(int amount) { healAmount_ = amount; }

private:
	int healAmount_ = 30;            // 回復電力
	bool isCollected_ = false;

	float waterSurfaceY_ = 0.2f;     // 水面位置
	float floatTimer_ = 0.0f;        // 浮遊・点滅タイマー
	float lifetime_ = 60.0f;         // 60秒で自然消滅
	float lifeTimer_ = 0.0f;
};
