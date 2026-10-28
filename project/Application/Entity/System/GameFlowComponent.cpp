#include "PCH.h"
#include "GameFlowComponent.h"
#include "BoatDriverComponent.h"
#include "GameObject.h"
#include "BaseScene.h"
#include "SceneManager.h"
#include "PlayScene.h"
#include "TitleScene.h"
#include "Time.h"
#include "InputManager.h"
#include "RawInput.h"
#include "GamePad.h"

float GameFlowComponent::sessionBest_ = 0.0f;

void GameFlowComponent::Update() {
	if (!gameObject_) return;
	if (!FindReferences()) return;

	// Initialize の時点ではほかのオブジェクトが読み込み途中のことがあるので、最初の Update で準備する
	if (!isStarted_) { StartRace(); }

	phaseTime_ += Time::GetDeltaTime();

	// 最後のジャンプ中にゴールしても高さを記録できるように、Phase に関係なく毎フレーム数える
	TrackJumps();

	switch (phase_) {
	case Phase::Ready:   UpdateReady();   break;
	case Phase::Playing: UpdatePlaying(); break;
	case Phase::Ending:  UpdateEnding();  break;
	case Phase::Result:  UpdateResult();  break;
	}
}

bool GameFlowComponent::FindReferences() {
	if (!boarder_) { boarder_ = gameObject_->FindComponentInScene<BoarderComponent>(); }
	if (!boat_) { boat_ = gameObject_->FindComponentInScene<BoatDriverComponent>(); }
	return boarder_ && boat_;
}

void GameFlowComponent::StartRace() {
	isStarted_ = true;

	// 同じフレームで船の Update が先に走って少し動いているかもしれないので、スタート地点に戻す
	boat_->ResetToStart();
	boat_->SetRunning(false);
	boarder_->SetSwingEnabled(false);
	boarder_->SetJumpEnabled(false);

	jumpsUsed_ = 0;
	jumpHeights_.clear();
	endReason_ = EndReason::None;
	prevBest_ = sessionBest_;
	isNewRecord_ = false;
	prevBoarderState_ = boarder_->GetState();
	ChangePhase(Phase::Ready);
}

void GameFlowComponent::TrackJumps() {
	BoarderComponent::State state = boarder_->GetState();

	// 踏み切り：回数を使う。使い切ったらもう跳べないようにする
	// （GameFlow とボーダーの Update 順によっては「着水 → 同じフレームでもう1回踏み切り」が起きうるので、ここで塞ぐ）
	if (prevBoarderState_ == BoarderComponent::State::Riding && state == BoarderComponent::State::Airborne) {
		jumpsUsed_++;
		if (jumpsUsed_ >= maxJumps_) {
			boarder_->SetJumpEnabled(false);
		}
	}
	// 着水：高さを記録
	if (prevBoarderState_ == BoarderComponent::State::Airborne && state == BoarderComponent::State::Riding) {
		float height = boarder_->GetLastJumpHeight();
		jumpHeights_.push_back(height);
		if (height > prevBest_) { isNewRecord_ = true; }
		sessionBest_ = (std::max)(sessionBest_, height);
	}
	prevBoarderState_ = state;
}

void GameFlowComponent::ChangePhase(Phase next) {
	phase_ = next;
	phaseTime_ = 0.0f;
}

void GameFlowComponent::EndRace(EndReason reason) {
	endReason_ = reason;
	boarder_->SetJumpEnabled(false);
	boat_->SetRunning(false); // 強制終了のときも船を減速させて「終わった感」を出す
	ChangePhase(Phase::Ending);
}

void GameFlowComponent::UpdateReady() {
	if (phaseTime_ >= countdownTime_) {
		boat_->SetRunning(true);
		boarder_->SetSwingEnabled(true);
		boarder_->SetJumpEnabled(true);
		ChangePhase(Phase::Playing);
	}
}

void GameFlowComponent::UpdatePlaying() {
	// ゴール優先（最後のジャンプ中にゴールしても GOAL 扱い。高さは着水時に記録される）
	if (boat_->IsFinished()) {
		EndRace(EndReason::Goal);
		return;
	}
	// 回数を使い切って、最後のジャンプが着水した
	if (jumpsUsed_ >= maxJumps_ && boarder_->GetState() == BoarderComponent::State::Riding) {
		EndRace(EndReason::OutOfJumps);
	}
}

void GameFlowComponent::UpdateEnding() {
	// 空中ならまだ数えない（着水して高さが確定してから endingTime_ 秒）
	if (boarder_->GetState() == BoarderComponent::State::Airborne) {
		phaseTime_ = 0.0f;
		return;
	}
	if (phaseTime_ >= endingTime_) {
		boarder_->SetSwingEnabled(false);
		ChangePhase(Phase::Result);
	}
}

void GameFlowComponent::UpdateResult() {
	if (phaseTime_ < resultInputDelay_) return;

	InputManager* input = InputManager::GetInstance();
	GamePad* pad = input->GetGamePad();
	bool padOk = pad->IsConection();

	// ESC はポーズに使っているので、タイトルへは BackSpace / B
	bool retry = input->GetRawInput()->Trigger(VK_SPACE) || (padOk && pad->TriggerButton(Button::A));
	bool title = input->GetRawInput()->Trigger(VK_BACK) || (padOk && pad->TriggerButton(Button::B));

	SceneContext* context = gameObject_->GetContext();
	SceneManager* sceneManager = context ? context->sceneManager : nullptr;
	if (!sceneManager || sceneManager->isTransitioning()) return;

	if (retry) {
		sceneManager->ChangeSceneWithDissolve<PlayScene>(0.5f, 0.5f);
	}
	else if (title) {
		sceneManager->ChangeSceneWithDissolve<TitleScene>(0.8f, 0.8f);
	}
}

float GameFlowComponent::GetBestHeight() const {
	float best = 0.0f;
	for (float h : jumpHeights_) { best = (std::max)(best, h); }
	return best;
}

void GameFlowComponent::ImGui() {
#ifdef USEIMGUI
	ImGui::Text("--- ゲーム進行 ---");
	ImGui::SliderInt("Max Jumps", &maxJumps_, 1, 10);
	ImGui::DragFloat("Countdown Time", &countdownTime_, 0.1f, 0.0f, 10.0f);
	ImGui::DragFloat("Ending Time", &endingTime_, 0.1f, 0.0f, 10.0f);
	ImGui::DragFloat("Result Input Delay", &resultInputDelay_, 0.05f, 0.0f, 3.0f);

	static const char* kPhaseNames[] = { "Ready", "Playing", "Ending", "Result" };
	ImGui::Text("phase: %s (%.2f s)", kPhaseNames[static_cast<int>(phase_)], phaseTime_);
	ImGui::Text("jumps: %d / %d  best %.1f m  session best %.1f m", jumpsUsed_, maxJumps_, GetBestHeight(), sessionBest_);
	ImGui::Text("boat: %s  boarder: %s", boat_ ? "found" : "none", boarder_ ? "found" : "none");
	if (ImGui::Button("Reset Session Best")) { sessionBest_ = 0.0f; }
#endif
}

void GameFlowComponent::Serialize(json& j) const {
	j["type"] = "GameFlowComponent";
	j["maxJumps"] = maxJumps_;
	j["countdownTime"] = countdownTime_;
	j["endingTime"] = endingTime_;
	j["resultInputDelay"] = resultInputDelay_;
}

void GameFlowComponent::Deserialize(const json& j) {
	if (j.contains("maxJumps")) maxJumps_ = j["maxJumps"];
	if (j.contains("countdownTime")) countdownTime_ = j["countdownTime"];
	if (j.contains("endingTime")) endingTime_ = j["endingTime"];
	if (j.contains("resultInputDelay")) resultInputDelay_ = j["resultInputDelay"];
}
