#pragma once
#include "struct.h"
#include "../magosuya/MagosuyaEngine.h"

class Player;

class PlayerState
{
public:
	PlayerState() = default;
	virtual ~PlayerState() = default;
public:
	virtual void Initialize() = 0;
	virtual void Update() = 0;
	// 終了時に呼び出す処理
	virtual void Exit() {};
	void SetPlayer(Player* player) { player_ = player; }
protected:
	Player* player_ = nullptr;
};

class PlayerStopState
	:public PlayerState
{
public:
	void Initialize()override;
	void Update()override;
private:

};

class PlayerWalkState
	:public PlayerState
{
public:
	void Initialize()override;
	void Update()override;
private:

};

class PlayerFallState
	:public PlayerState
{
public:
	void Initialize()override;
	void Update()override;
private:
	// 落ちているときの挙動とかetc...

};

class PlayerDashState
	:public PlayerState
{
public:
	void Initialize()override;
	void Update()override;
	void Exit()override;
private:
	// 加速に関して
	float currentSpeedMultiplier_ = 0.0f;// 現在の速度倍率
	float initial_speedMultiplier_ = 1.0f;// ダッシュ開始時の速度
	float maxSpeedMultiplier_ = 1.5f;// ダッシュの最大速度
	float accelerationRate_ = 0.9f;// １秒当たりの速度上昇率
	// スタミナに関して
	float initial_staminaCost_ = 10.0f;// ダッシュ開始時の固定消費
	float staminaDrainRate_ = 15.0f;// １秒あたりの継続消費量
};

// スタミナ切れ時の State
class PlayerExhaustedState
	: public PlayerState // PlayerStateを継承
{
public:
	void Initialize() override;
	void Update() override;
	void Exit() override;
private:
	float STAMINA_REGEN_DELAY = 1.0f; // スタミナ回復が始まるまでの遅延時間
	float exhaustedTimer_ = 0.0f; // 遅延時間を計測するタイマー
	float EXHAUSTED_REGEN_THRESHOLD = 70.0f; // 復帰に必要なスタミナの閾値（例: 30%）
};

// ** 戦闘系のアクション **

class PlayerAttackState
	:public PlayerState
{
public:
	void Initialize() override;
	void Update() override;
	void Exit() override;
private:
	float attackTimer_ = 0.0f;          // 攻撃アニメーションの経過時間
	float maxAttackDuration_ = 0.5f; // 攻撃アニメーションの持続時間
	// コンボ受付期間を管理する変数
	bool canCombo_ = false;
	float comboWindowStart_ = 0.2f; // 攻撃開始から200ms後
	float comboWindowEnd_ = 0.4f;   // 攻撃開始から400ms後
};

class PlayerComboAttackState
	:public PlayerState
{
public:
	void Initialize()override;
	void Update()override;
private:
	// コンボ段階の管理、次の入力受付、コンボ終了後の遷移
};

class PlayerEvasionState
	: public PlayerState
{
public:
	void Initialize() override;
	void Update() override;
	void Exit() override {} // Exit時は特に処理なし

private:
	float evasionTimer_ = 0.0f;               // 回避の経過時間
	float EVASION_DURATION = 0.3f;      // 回避の持続時間（0.5秒）
	float EVASION_SPEED_RATE = 12.0f;        // 一回で進む距離
	float initial_verticalVelocity = 0.75f;// 縦に飛ぶ距離
	float targetAngle_ = 360.0f;          // 回転させたい量
	float initial_staminaCost_ = 10.0f;// 回避開始時の固定消費

	Vector3 evasionDirection_{ 0.0f, 0.0f, 0.0f }; // 回避する方向のベクトル
	//Quaternion preQuaternion_{};        // 回避する瞬間の回転を保持（将来バグの原因の可能性あり）
};

class PlayerDeathState
	: public PlayerState
{
public:
	void Initialize() override;
	void Update() override;
	void Exit() override {} // 死亡 State から Exit する際は特別な処理は不要（シーン遷移を想定）

private:
	float deathTimer_ = 0.0f;           // 死亡演出の経過時間
	float MAX_DEATH_DURATION = 3.0f; // 死亡演出の持続時間（3秒間を想定）
};

class PlayerHurtState
	: public PlayerState
{
public:
	void Initialize() override;
	void Update() override;
	void Exit() override;

private:
	float hurtTimer_ = 0.0f;          // のけぞりの経過時間
	float MAX_HURT_DURATION = 0.8f;
};

