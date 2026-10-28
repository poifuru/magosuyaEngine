#pragma once
#include "Component.h"
#include "BoarderComponent.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class FontTextComponent;

// ボーダーのジャンプの高さを、同じ GameObject の FontText に表示する
// ・空中：今のジャンプでここまでの最高点（上がっていき、頂点で止まる）
// ・着水後：確定した高さを holdTime_ 秒だけ残す。記録更新なら "NEW RECORD!" を付ける
// ・滑っている間：何も出さない
class JumpHeightHudComponent : public Component {
public:
	JumpHeightHudComponent() = default;
	~JumpHeightHudComponent() override = default;

	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "JumpHeightHudComponent"; }

private:
	// ===== 調整パラメータ（シリアライズ対象） =====
	float holdTime_ = 1.5f; // 着水後に結果を残す時間[s]

	// ===== 実行時の状態 =====
	BoarderComponent* boarder_ = nullptr; // シーン内のボーダー（最初に1回だけ探す）
	FontTextComponent* text_ = nullptr;   // 同じ GameObject の FontText
	BoarderComponent::State prevState_ = BoarderComponent::State::Riding;
	float holdTimer_ = 0.0f;
	float prevBest_ = 0.0f;   // 踏み切った時点での最高記録
	bool isNewRecord_ = false;
};
