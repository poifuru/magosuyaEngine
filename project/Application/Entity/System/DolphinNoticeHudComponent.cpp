#include "PCH.h"
#include "DolphinNoticeHudComponent.h"
#include "DolphinEventComponent.h"
#include "GameObject.h"
#include "FontTextComponent.h"

void DolphinNoticeHudComponent::Update() {
	if (!gameObject_) return;

	if (!text_) { text_ = gameObject_->GetComponent<FontTextComponent>(); }
	if (!dolphin_) { dolphin_ = gameObject_->FindComponentInScene<DolphinEventComponent>(); } // 無いステージもある
	if (!text_) return;

	// イルカが近づいてくる間（予兆）だけ出す（表示が変わらなければ SetText は頂点を作り直さない）
	std::string str;
	if (dolphin_ && dolphin_->GetState() == DolphinEventComponent::State::Approach) {
		str = dolphin_->GetArrivalText();
	}
	text_->SetText(str);
}

void DolphinNoticeHudComponent::ImGui() {
#ifdef USEIMGUI
	ImGui::Text("--- イルカ告知 HUD ---");
	ImGui::Text("dolphin: %s  font text: %s", dolphin_ ? "found" : "none", text_ ? "found" : "none");
#endif
}

void DolphinNoticeHudComponent::Serialize(json& j) const {
	j["type"] = "DolphinNoticeHudComponent";
}

void DolphinNoticeHudComponent::Deserialize(const json& j) {
}
