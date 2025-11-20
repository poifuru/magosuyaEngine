#include "PlayerState.h"
#include "../Player.h"

void PlayerFallState::Initialize() {
	
}

void PlayerFallState::Update() {

	if (player_->IsOnGround() == true) {
		// 地面についていたら一旦止まる
		player_->ChangeState(new PlayerStopState());
		return;
	}
}