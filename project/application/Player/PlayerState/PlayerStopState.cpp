#include "PlayerState.h"
#include "../Player.h"

void PlayerStopState::Initialize() {

}

void PlayerStopState::Update() {

	bool isMove = false;
	if (player_->engine_->GetRawInput()->Push('W')) { isMove = true; }
	else if (player_->engine_->GetRawInput()->Push('S')) { isMove = true; }
	else if (player_->engine_->GetRawInput()->Push('A')) { isMove = true; }
	else if (player_->engine_->GetRawInput()->Push('D')) { isMove = true; }

	bool has_stamina = player_->GetStamina() > 0.0f;

	if (has_stamina == false) {
		// スタミナ切れ
		player_->ChangeState(new PlayerExhaustedState());
		return;
	}

	if (isMove) {
		player_->ChangeState(new PlayerWalkState());
		return;
	}

	//if (InputManager::IsDash()) {
	//	// 走る
	//	player_->ChangeState(new PlayerDashState());
	//	return;
	//}

	if (player_->engine_->GetRawInput()->Push('J')) {
		// 攻撃
		player_->ChangeState(new PlayerAttackState());
		return;
	}
}