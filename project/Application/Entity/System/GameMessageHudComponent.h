#pragma once
#include "Component.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class FontTextComponent;
class GameFlowComponent;

// 画面中央のメッセージを、同じ GameObject の FontText に表示する
// ・Ready：3 → 2 → 1
// ・Playing：最初の goTime_ 秒だけ "GO!"
// ・Ending："GOAL!" / "FINISH!"
// ・Result：各ジャンプの高さと最高・平均、操作説明
class GameMessageHudComponent : public Component {
public:
	GameMessageHudComponent() = default;
	~GameMessageHudComponent() override = default;

	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "GameMessageHudComponent"; }

private:
	// ===== 調整パラメータ（シリアライズ対象） =====
	float goTime_ = 1.0f; // "GO!" を出す時間[s]

	// ===== 実行時の状態 =====
	GameFlowComponent* flow_ = nullptr;   // シーン内の GameFlow（最初に1回だけ探す）
	FontTextComponent* text_ = nullptr;   // 同じ GameObject の FontText
};
