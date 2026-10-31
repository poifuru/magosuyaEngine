#include "PCH.h"
#include "JumpHeightHudComponent.h"
#include "GameObject.h"
#include "FontTextComponent.h"
#include "Time.h"
#include <format>

void JumpHeightHudComponent::Update() {
	if (!gameObject_) return;

	// 参照は最初に1回だけ探して覚えておく（毎フレーム全オブジェクトを走査しない）
	if (!text_) { text_ = gameObject_->GetComponent<FontTextComponent>(); }
	if (!boarder_) { boarder_ = gameObject_->FindComponentInScene<BoarderComponent>(); }
	if (!text_ || !boarder_) return;

	float dt = Time::GetDeltaTime();
	BoarderComponent::State state = boarder_->GetState();

	// --- 状態が切り替わった瞬間を拾う ---
	if (prevState_ == BoarderComponent::State::Riding && state == BoarderComponent::State::Airborne) {
		// 踏み切り：前のジャンプの表示を消す
		holdTimer_ = 0.0f;
	}
	if (prevState_ == BoarderComponent::State::Airborne && state == BoarderComponent::State::Riding) {
		// 着水：確定した高さをしばらく残す
		holdTimer_ = holdTime_;
	}
	prevState_ = state;

	// --- 表示する文字列を作る ---
	// 小数1桁に丸めているので、SetText が頂点を作り直すのは表示が変わったフレームだけ
	std::string str;
	if (state == BoarderComponent::State::Airborne) {
		str = std::format("{:.1f} m", boarder_->GetCurrentJumpHeight());
	}
	else if (holdTimer_ > 0.0f) {
		holdTimer_ -= dt;
		str = std::format("{:.1f} m", boarder_->GetLastJumpHeight());
	}
	// それ以外は空文字 = 何も出ない

	text_->SetText(str);
}

void JumpHeightHudComponent::ImGui() {
#ifdef USEIMGUI
	ImGui::Text("--- ジャンプ高度 HUD ---");
	ImGui::DragFloat("Hold Time", &holdTime_, 0.05f, 0.0f, 10.0f);
	ImGui::Text("boarder: %s  font text: %s", boarder_ ? "found" : "none", text_ ? "found" : "none");
#endif
}

void JumpHeightHudComponent::Serialize(json& j) const {
	j["type"] = "JumpHeightHudComponent";
	j["holdTime"] = holdTime_;
}

void JumpHeightHudComponent::Deserialize(const json& j) {
	if (j.contains("holdTime")) holdTime_ = j["holdTime"];
}
