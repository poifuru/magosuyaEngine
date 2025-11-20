#include "PlayerState.h"
#include "../Player.h"

void PlayerDashState::Initialize() {
	
	// もしダッシュできない状態ならどうする？
	if (player_->GetStamina() < 0.0f) {
		// 疲れた状態への遷移？ or 歩く？

	}
	// 開始時のスタミナの消費
	player_->DrainStamina(initial_staminaCost_);

	// 回復をブロック
	player_->BlockStaminaRecovery();

	// 加速の起点
	currentSpeedMultiplier_ = initial_speedMultiplier_;
	player_->SetSpeedMultiplier(currentSpeedMultiplier_);
}

void PlayerDashState::Update() {

	bool isMove = false;
	auto& move = player_->Move();
	if (player_->engine_->GetRawInput()->Push('W'))
	{
		move.z = 1.0f;
		isMove = true;
	}
	if (player_->engine_->GetRawInput()->Push('S'))
	{
		move.z = -1.0f;
		isMove = true;
	}
	if (player_->engine_->GetRawInput()->Push('A'))
	{
		move.x = -1.0f;
		isMove = true;
	}
	if (player_->engine_->GetRawInput()->Push('D'))
	{
		move.x = 1.0f;
		isMove = true;
	}

	const float deltaTime = 1.0f / 60.0f;

	// 加速処理
	if (currentSpeedMultiplier_ < maxSpeedMultiplier_) {
		currentSpeedMultiplier_ += accelerationRate_ * deltaTime;
		if (currentSpeedMultiplier_ > maxSpeedMultiplier_) {
			// 最大を超えたときの処理
			currentSpeedMultiplier_ = maxSpeedMultiplier_;
		}
	}
	// 適用
	player_->SetSpeedMultiplier(currentSpeedMultiplier_);

	// スタミナを減少
	player_->DrainStamina(staminaDrainRate_ * deltaTime);

	//	ここから状態遷移チェック
	bool is_dash = false;
	bool has_stamina = player_->GetStamina() > 0.0f;

	if (has_stamina == false) {
		// スタミナ切れ
		player_->ChangeState(new PlayerExhaustedState());
		return;
	}

	if (is_dash == false) {
		player_->ChangeState(new PlayerStopState());
		return;
	}

	if (player_->IsOnGround() == false) {
		// 空中にいる場合の処理
		player_->ChangeState(new PlayerFallState());
		return;
	}

	if (player_->engine_->GetRawInput()->Push('J')) {
		// 攻撃
		player_->ChangeState(new PlayerAttackState());
		return;
	}
	
	//ImGuiManager::GetInstance()->Text("DashState");
}

void PlayerDashState::Exit() {
	// スタミナの回復ブロックを解除
	player_->UnblockStaminaRecovery();

	// 速度を戻す
	player_->SetSpeedMultiplier(1.0f);

	// なにかあればここに書く

}