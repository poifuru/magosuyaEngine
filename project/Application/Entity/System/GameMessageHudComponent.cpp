#include "PCH.h"
#include "GameMessageHudComponent.h"
#include "GameFlowComponent.h"
#include "GameObject.h"
#include "FontTextComponent.h"
#include <cmath>
#include <format>

void GameMessageHudComponent::Update() {
	if (!gameObject_) return;

	if (!text_) { text_ = gameObject_->GetComponent<FontTextComponent>(); }
	if (!flow_) { flow_ = gameObject_->FindComponentInScene<GameFlowComponent>(); }
	if (!text_ || !flow_) return;

	using Phase = GameFlowComponent::Phase;
	using EndReason = GameFlowComponent::EndReason;
	float t = flow_->GetPhaseTime();
	std::string str;

	switch (flow_->GetPhase()) {
	case Phase::Ready: {
		// 3 → 2 → 1（残り時間の切り上げ）
		int count = static_cast<int>(std::ceil(flow_->GetCountdownTime() - t));
		if (count > 0) { str = std::to_string(count); }
		// イルカの予告はステージセレクトで出すので、ここでは出さない
		break;
	}
	case Phase::Playing:
		if (t < goTime_) { str = "GO!"; }
		// イルカの告知は DolphinNoticeHudComponent が左上に出す
		break;
	case Phase::Ending:
		str = (flow_->GetEndReason() == EndReason::Goal) ? "GOAL!" : "FINISH!";
		break;
	case Phase::Result: {
		str = (flow_->GetEndReason() == EndReason::Goal) ? "GOAL!\n\n" : "ジャンプ終了\n\n";
		const auto& heights = flow_->GetJumpHeights();
		for (size_t i = 0; i < heights.size(); ++i) {
			str += std::format("JUMP {}   {:.1f} m\n", i + 1, heights[i]);
		}
		str += std::format("\n最高 {:.1f} m\n", flow_->GetBestHeight());
		str += std::format("平均 {:.1f} m\n", flow_->GetAverageHeight());
		str += "\nA / Space : 再挑戦\nB / BackSpace : ステージ選択";
		break;
	}
	}

	text_->SetText(str);
}

void GameMessageHudComponent::ImGui() {
#ifdef USEIMGUI
	ImGui::Text("--- メッセージ HUD ---");
	ImGui::DragFloat("GO Time", &goTime_, 0.05f, 0.0f, 5.0f);
	ImGui::Text("game flow: %s  font text: %s", flow_ ? "found" : "none", text_ ? "found" : "none");
#endif
}

void GameMessageHudComponent::Serialize(json& j) const {
	j["type"] = "GameMessageHudComponent";
	j["goTime"] = goTime_;
}

void GameMessageHudComponent::Deserialize(const json& j) {
	if (j.contains("goTime")) goTime_ = j["goTime"];
}
