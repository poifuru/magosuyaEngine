#include "PlayerState.h"
#include "Player3D.h"

void PlayerEvasionState::Initialize()
{
	if (!player_) return;

	// 1. **タイマーのリセット**
	evasionTimer_ = 0.0f;

	// 2. **回避方向の決定**
	// 入力された移動方向を取得。入力がない場合は、現在向いている前方方向を回避方向とする。
	Vector3 inputVector;

	auto& move = player_->Move();
	if (InputManager::GetKey().PressKey(DIK_W)){
		move.z = 1.0f;
	}
	if (InputManager::GetKey().PressKey(DIK_S)){
		move.z = -1.0f;
	}
	if (InputManager::GetKey().PressKey(DIK_A)){
		move.x = -1.0f;
	}
	if (InputManager::GetKey().PressKey(DIK_D)){
		move.x = 1.0f;
	}
	inputVector = move;

	if (Length(inputVector) > 0.001f){
		// 入力がある場合、入力方向に回避
		evasionDirection_ = Normalize(inputVector);
	}
	else{
		// 入力がない場合、プレイヤーの正面方向に回避
		evasionDirection_ = player_->GetForwardVector();
	}

	// 3. **無敵状態の設定**
	player_->SetInvulnerable(true);

	// 4. **回避アニメーションの再生**
	// player_->PlayAnimation("Evasion_Roll");

	// スタミナを減らす
	player_->BlockStaminaRecovery();
	player_->DrainStamina(initial_staminaCost_);

	// 空中に浮く
	player_->SetVerticalVelocity(initial_verticalVelocity);
	player_->SetIsOnGround(false);

	// 回転を保存
	preQuaternion_ = player_->GetPlayerQuaternion();
}

void PlayerEvasionState::Update()
{
	if (!player_) return;

	const float deltaTime = 1.0f / 60.0f;
	evasionTimer_ += deltaTime;

	// 1. **移動処理**
	// 回避方向に高速移動を適用
	player_->Move() = evasionDirection_;
	player_->SetSpeedMultiplier((EVASION_SPEED_RATE / EVASION_DURATION) * deltaTime);

	// 回転処理
	float t = evasionTimer_ / EVASION_DURATION;
	if (t >= 1.0f) {
		t = 1.0f;
	}
	float currentAngle = Deg2Rad(targetAngle_) * t;

	// 向いている方向とワールドの真上の軸から回転させる軸を生成
	Vector3 rotationAxis = CrossProduct({ 0.0f,1.0f,0.0f }, player_->GetForwardVector());
	rotationAxis = Normalize(rotationAxis);

	// Quaternionの生成
	Quaternion evasionRotation = Quaternion::MakeRotateAxisAngleQuaternion(rotationAxis, currentAngle);

	player_->SetPlayerQuaternion(evasionRotation);

	// 2. **状態遷移ロジック (終了判定)**

	// --- 回避時間終了 ---
	if (evasionTimer_ >= EVASION_DURATION)
	{
		// 無敵状態を解除
		player_->SetInvulnerable(false);

		// 速度を元に戻す
		player_->SetSpeedMultiplier(1.0f);

		// 回転をもとに戻す
		player_->SetPlayerQuaternion(preQuaternion_);

		// スタミナ回復を解除
		player_->UnblockStaminaRecovery();

		// 待機 State に戻る
		player_->ChangeState(new PlayerStopState());
		return;
	}

	ImGuiManager::GetInstance()->Text("EvasionState");
}