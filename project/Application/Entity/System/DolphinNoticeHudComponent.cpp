#include "PCH.h"
#include "DolphinNoticeHudComponent.h"
#include "DolphinEventComponent.h"
#include "RoughSeaComponent.h"
#include "GameObject.h"
#include "FontTextComponent.h"

void DolphinNoticeHudComponent::Update() {
	if (!gameObject_) return;

	if (!text_) { text_ = gameObject_->GetComponent<FontTextComponent>(); }
	if (!dolphin_) { dolphin_ = gameObject_->FindComponentInScene<DolphinEventComponent>(); } // 無いステージもある
	if (!roughSea_) { roughSea_ = gameObject_->FindComponentInScene<RoughSeaComponent>(); }
	if (!text_) return;

	// イルカが近づいてくる間（予兆）だけ出す（表示が変わらなければ SetText は頂点を作り直さない）
	std::string str;
	if (dolphin_ && dolphin_->GetState() == DolphinEventComponent::State::Approach) {
		str = dolphin_->GetArrivalText();
	}
	// うねりの予告（予告〜大きくなる間）
	if (str.empty() && roughSea_) {
		auto s = roughSea_->GetState();
		if (s == RoughSeaComponent::State::Warning || s == RoughSeaComponent::State::Rising) {
			str = roughSea_->GetWarningText();
		}
	}
	text_->SetText(str);
}

void DolphinNoticeHudComponent::ImGui() {
#ifdef USEIMGUI
	ImGui::Text("--- イベント告知 HUD（イルカ・うねり） ---");
	ImGui::Text("イルカ: %s  うねり: %s  文字: %s", dolphin_ ? "あり" : "なし", roughSea_ ? "あり" : "なし", text_ ? "あり" : "なし");
#endif
}

void DolphinNoticeHudComponent::Serialize(json& j) const {
	j["type"] = "DolphinNoticeHudComponent";
}

void DolphinNoticeHudComponent::Deserialize(const json& j) {
}
