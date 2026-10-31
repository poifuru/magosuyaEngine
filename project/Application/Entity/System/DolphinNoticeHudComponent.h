#pragma once
#include "Component.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class FontTextComponent;
class DolphinEventComponent;

// イルカの告知を、同じ GameObject の FontText に表示する（画面の左上などに置く想定）
// ・イルカが近づいてくる間（Approach）だけ DolphinEventComponent::GetArrivalText() を出す
// ・イルカがいないステージでは何も出さない
class DolphinNoticeHudComponent : public Component {
public:
	DolphinNoticeHudComponent() = default;
	~DolphinNoticeHudComponent() override = default;

	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "DolphinNoticeHudComponent"; }

private:
	// ===== 実行時の状態 =====
	DolphinEventComponent* dolphin_ = nullptr; // シーン内のイルカイベント（最初に1回だけ探す）
	FontTextComponent* text_ = nullptr;        // 同じ GameObject の FontText
};
