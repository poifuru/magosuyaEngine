#include "PlayerState.h"
#include "Player3D.h"

void PlayerFallState::Initialize() {
	
}

void PlayerFallState::Update() {

	if (player_->IsOnGround() == true) {
		// 地面についていたら一旦止まる
		player_->ChangeState(new PlayerStopState());
		return;
	}

	if (player_->CanDoubleJump() && InputManager::IsJump()) {
		// 二段ジャンプへ
		player_->ChangeState(new PlayerDoubleJumpState());
		return;
	}

	ImGuiManager::GetInstance()->Text("FallState");
}