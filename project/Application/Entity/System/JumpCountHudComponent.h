#pragma once
#include "Component.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class FontTextComponent;
class GameFlowComponent;

// 残りジャンプ数を、同じ GameObject の FontText に「JUMP ●●○」の形で表示する
// ・● = 残り、○ = 使った
// ・リザルト中は消す
class JumpCountHudComponent : public Component {
public:
	JumpCountHudComponent() = default;
	~JumpCountHudComponent() override = default;

	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "JumpCountHudComponent"; }

private:
	// ===== 実行時の状態 =====
	GameFlowComponent* flow_ = nullptr;   // シーン内の GameFlow（最初に1回だけ探す）
	FontTextComponent* text_ = nullptr;   // 同じ GameObject の FontText
};
