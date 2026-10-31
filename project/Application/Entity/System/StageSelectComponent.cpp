#include "PCH.h"
#include "StageSelectComponent.h"
#include "GameObject.h"
#include "BaseScene.h"
#include "SceneManager.h"
#include "PlayScene.h"
#include "TitleScene.h"
#include "FontTextComponent.h"
#include "EditorManager.h"
#include "InputManager.h"
#include "RawInput.h"
#include "GamePad.h"
#include <format>

void StageSelectComponent::Update() {
	if (!gameObject_) return;

	// Initialize の時点ではほかのオブジェクトが読み込み途中のことがあるので、最初の Update で準備する
	if (!isLoaded_) { LoadStages(); }
	FindReferences();

	if (CanAcceptInput()) { UpdateInput(); }
	UpdateText();
}

void StageSelectComponent::LoadStages() {
	isLoaded_ = true;
	stages_ = StageSelection::LoadStageList();

	// リザルトから戻ってきたときは、前に選んだステージにカーソルを合わせる
	cursor_ = StageSelection::GetSelectedIndex();
	if (cursor_ < 0 || cursor_ >= static_cast<int>(stages_.size())) { cursor_ = 0; }
}

void StageSelectComponent::FindReferences() {
	if (!listText_) { listText_ = gameObject_->GetComponent<FontTextComponent>(); }
	if (!detailText_) {
		SceneContext* context = gameObject_->GetContext();
		if (context && context->activeGameObjects) {
			for (auto& obj : *context->activeGameObjects) {
				if (obj->GetName() == detailObjectName_) {
					detailText_ = obj->GetComponent<FontTextComponent>();
					break;
				}
			}
		}
	}
}

bool StageSelectComponent::CanAcceptInput() const {
#ifdef USEIMGUI
	// エディタで停止中（編集中）は、キー入力でシーンが変わらないようにする
	if (!EditorManager::GetInstance()->IsPlaying()) return false;
#endif
	SceneContext* context = gameObject_->GetContext();
	return context && context->sceneManager && !context->sceneManager->isTransitioning();
}

void StageSelectComponent::UpdateInput() {
	InputManager* input = InputManager::GetInstance();
	RawInput* key = input->GetRawInput();
	GamePad* pad = input->GetGamePad();
	bool padOk = pad->IsConnection();

	// スティックは「倒した瞬間」だけ1つ動かす
	int stickDir = 0;
	if (padOk) {
		float y = pad->GetStick(LR::Left).y;
		if (y > stickThreshold_) { stickDir = 1; }
		else if (y < -stickThreshold_) { stickDir = -1; }
	}
	bool stickUp = (stickDir == 1 && prevStickDir_ != 1);
	bool stickDown = (stickDir == -1 && prevStickDir_ != -1);
	prevStickDir_ = stickDir;

	bool up = key->Trigger(VK_UP) || key->Trigger('W') || stickUp || (padOk && pad->TriggerButton(Button::CROSS_UP));
	bool down = key->Trigger(VK_DOWN) || key->Trigger('S') || stickDown || (padOk && pad->TriggerButton(Button::CROSS_DOWN));
	if (up) { MoveCursor(-1); }
	if (down) { MoveCursor(1); }

	// ESC はポーズに使っているので、戻るは BackSpace / B（リザルトと同じ）
	bool decide = key->Trigger(VK_SPACE) || key->Trigger(VK_RETURN) || (padOk && pad->TriggerButton(Button::A));
	bool back = key->Trigger(VK_BACK) || (padOk && pad->TriggerButton(Button::B));

	SceneManager* sceneManager = gameObject_->GetContext()->sceneManager;
	if (decide) {
		Decide();
	}
	else if (back) {
		sceneManager->ChangeSceneWithDissolve<TitleScene>(0.8f, 0.8f);
	}
}

void StageSelectComponent::MoveCursor(int delta) {
	int count = static_cast<int>(stages_.size());
	if (count == 0) return;
	// 端まで行ったら反対側へ回る
	cursor_ = (cursor_ + delta + count) % count;
}

void StageSelectComponent::Decide() {
	if (stages_.empty()) return;
	const StageInfo& stage = stages_[cursor_];
	if (!stage.sceneExists) return; // シーンが無いステージは選べない

	StageSelection::Select(cursor_, stage);
#ifdef USEIMGUI
	// TitleScene と同じく、ゲームを始めるのでエディタも再生状態にする
	EditorManager::GetInstance()->Play();
#endif
	gameObject_->GetContext()->sceneManager->ChangeSceneWithDissolve<PlayScene>(0.8f, 0.8f);
}

void StageSelectComponent::UpdateText() {
	if (!listText_) return;

	// ===== 一覧 =====
	std::string list = "ステージ選択\n\n";
	if (stages_.empty()) {
		list += "ステージがありません\n(" + StageSelection::kStageListPath + ")\n";
	}
	for (int i = 0; i < static_cast<int>(stages_.size()); ++i) {
		const StageInfo& stage = stages_[i];
		list += (i == cursor_) ? "▶ " : "   ";
		list += stage.name;
		if (!stage.sceneExists) { list += "（準備中）"; }
		list += "\n";
	}
	list += "\nA / Space : 決定\nB / BackSpace : タイトル";

	// ===== 詳細 =====
	std::string detail;
	if (!stages_.empty()) {
		const StageInfo& stage = stages_[cursor_];
		detail = stage.name + "\n\n";
		if (stage.sceneExists) {
			detail += std::format("ジャンプ {} 回\n", stage.maxJumps);
			float best = StageRecords::GetBestHeight(stage.scenePath);
			detail += (best > 0.0f) ? std::format("自己ベスト {:.1f} m\n", best) : "自己ベスト ---\n";
		}
		else {
			detail += "準備中\n";
		}
		if (!stage.description.empty()) { detail += "\n" + stage.description + "\n"; }
		// イルカが来るステージなら予告する（どこで来るかは言わない）
		if (stage.hasDolphin) { detail += "\n" + stage.dolphinNotice + "\n"; }
	}

	// 詳細用の FontText が無ければ、一覧の下にまとめて出す
	if (detailText_) {
		listText_->SetText(list);
		detailText_->SetText(detail);
	}
	else {
		listText_->SetText(list + "\n\n" + detail);
	}
}

void StageSelectComponent::ImGui() {
#ifdef USEIMGUI
	ImGui::Text("--- ステージセレクト ---");
	char buf[128];
	strncpy_s(buf, detailObjectName_.c_str(), _TRUNCATE);
	if (ImGui::InputText("Detail Object", buf, sizeof(buf))) {
		detailObjectName_ = buf;
		detailText_ = nullptr; // 名前が変わったので探し直す
	}
	ImGui::SliderFloat("Stick Threshold", &stickThreshold_, 0.1f, 0.9f);
	ImGui::Text("stages: %d  cursor: %d", static_cast<int>(stages_.size()), cursor_);
	ImGui::Text("list text: %s  detail text: %s", listText_ ? "found" : "none", detailText_ ? "found" : "none");
	if (ImGui::Button("Reload stages.json")) { LoadStages(); }
#endif
}

void StageSelectComponent::Serialize(json& j) const {
	j["type"] = "StageSelectComponent";
	j["detailObjectName"] = detailObjectName_;
	j["stickThreshold"] = stickThreshold_;
}

void StageSelectComponent::Deserialize(const json& j) {
	if (j.contains("detailObjectName")) detailObjectName_ = j["detailObjectName"];
	if (j.contains("stickThreshold")) stickThreshold_ = j["stickThreshold"];
}
