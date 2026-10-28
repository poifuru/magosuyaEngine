#include "PCH.h"
#include "JumpCountHudComponent.h"
#include "GameFlowComponent.h"
#include "GameObject.h"
#include "FontTextComponent.h"

void JumpCountHudComponent::Update() {
	if (!gameObject_) return;

	if (!text_) { text_ = gameObject_->GetComponent<FontTextComponent>(); }
	if (!flow_) { flow_ = gameObject_->FindComponentInScene<GameFlowComponent>(); }
	if (!text_ || !flow_) return;

	// リザルト中は消す
	if (flow_->GetPhase() == GameFlowComponent::Phase::Result) {
		text_->SetText("");
		return;
	}

	// 残り = ●、使った = ○（表示が変わらなければ SetText は頂点を作り直さない）
	std::string str = "JUMP ";
	for (int i = 0; i < flow_->GetMaxJumps(); ++i) {
		str += (i < flow_->GetRemainingJumps()) ? "●" : "○";
	}
	text_->SetText(str);
}

void JumpCountHudComponent::ImGui() {
#ifdef USEIMGUI
	ImGui::Text("--- 残りジャンプ HUD ---");
	ImGui::Text("game flow: %s  font text: %s", flow_ ? "found" : "none", text_ ? "found" : "none");
#endif
}

void JumpCountHudComponent::Serialize(json& j) const {
	j["type"] = "JumpCountHudComponent";
}

void JumpCountHudComponent::Deserialize(const json& j) {
	(void)j;
}
