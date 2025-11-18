#include "PlayerState.h" 
#include "Player3D.h"

void PlayerHurtState::Initialize()
{
	if (!player_) return;

	// 1. **タイマーのリセット**
	hurtTimer_ = 0.0f;

	// 2. **無敵時間の開始**
	// Player3D::TakeDamageで既にtrueになっているが、念のため再度設定
	player_->SetInvulnerable(true);

	// 3. **被ダメージ演出の開始**
	// (例: のけぞりアニメーションの再生や、画面点滅の開始など)
}

void PlayerHurtState::Update()
{
	if (!player_) return;

	bool isMove = false;
	auto& move = player_->Move();
	if (InputManager::GetKey().PressKey(DIK_W))
	{
		move.z = 1.0f;
		isMove = true;
	}
	if (InputManager::GetKey().PressKey(DIK_S))
	{
		move.z = -1.0f;
		isMove = true;
	}
	if (InputManager::GetKey().PressKey(DIK_A))
	{
		move.x = -1.0f;
		isMove = true;
	}
	if (InputManager::GetKey().PressKey(DIK_D))
	{
		move.x = 1.0f;
		isMove = true;
	}

	// デルタタイム
	const float deltaTime = 1.0f / 60.0f;
	hurtTimer_ += deltaTime;

	int blinkCount = static_cast<int>(hurtTimer_ / 0.1f);
	
	if (blinkCount % 2 == 0){
		// 偶数回 (例: 0.0～0.1, 0.2～0.3) は不透明 (α=1.0)
		player_->SetAlpha(1.0f);
	}
	else{
		// 奇数回 (例: 0.1～0.2, 0.3～0.4) は透明 (α=0.0)
		player_->SetAlpha(0.0f);
	}

	// 2. **State 遷移ロジック (終了判定)**
	// --- のけぞりアニメーション/無敵時間終了 ---
	if (hurtTimer_ >= MAX_HURT_DURATION)
	{
		// 終了したら、待機 State に戻る
		player_->ChangeState(new PlayerStopState());
		return;
	}

	// 3. **死亡判定 (念のための再チェック)**
	if (player_->IsDead())
	{
		// 既に死亡していれば、死亡 State へ遷移
		player_->ChangeState(new PlayerDeathState());
		return;
	}

	ImGuiManager::GetInstance()->Text("HurtState");
}

void PlayerHurtState::Exit()
{
	if (!player_) return;
	// αを一応戻す
	player_->SetAlpha(1.0f);

	// 無敵時間を解除
	player_->SetInvulnerable(false);
}