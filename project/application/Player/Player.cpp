#include "Player.h"

Player::~Player() {
	delete state_;
}

void Player::Initialize(MagosuyaEngine* engine)
{
	obj_ = std::make_unique<Model>();
	obj_->Initialize();

	// 始まりのState
	state_ = new PlayerStopState();
	state_->SetPlayer(this);

	/*attackColliderObj_ = std::make_unique<ModelObject>();
	attackColliderObj_->Initialize(fngine->GetD3D12System(), "cube.obj");
	attackColliderObj_->SetFngine(fngine);
	attackColliderObj_->worldTransform_.set_.Scale({ 1.0f,1.0f,1.0f });
	attackColliderObj_->SetColor({ 0.0f,0.0f,0.0f,1.0f });
	attackColliderObj_->textureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");*/

	// Colliderの設定
	// 1, Player
	playerCollider_ = std::make_unique<PlayerBodyCollider>(this);
	// 2, Attack
	attackCollider_ = std::make_unique<AttackCollider>();
	EnableHitBox(false, obj_->GetTransform().translate);
}

void Player::Update()
{
	// 毎フレーム初期化する処理↓↓↓
	move_ = { 0.0f,0.0f,0.0f };

	// 毎フレーム初期化する処理↑↑↑

	ApplyPhysics();

	if (state_) {
		state_->Update();
	}

	UpdateStaminaRecovery();
	
	// カメラに対してにする処理
	//move_ = move_.z * CameraSystem::GetInstance()->GetActiveCamera()->zAxis_ + move_.x * CameraSystem::GetInstance()->GetActiveCamera()->xAxis_;
	// = Normalize(move_);
	move_.y = 1.0f;

	RotateToMoveDirection();

	Vector3 pos = obj_->GetTransform().translate;
	pos.x += move_.x * speed_ * speedMultiplier_;
	pos.y += move_.y * verticalVelocity_;
	pos.z += move_.z * speed_ * speedMultiplier_;
	obj_->SetTransform({obj_->GetTransform().rotate,obj_->GetTransform().scale,pos});
}

void Player::Draw()
{
	obj_->Draw();

	/*if (isAttackViewFlag_) {
		attackColliderObj_->worldTransform_.set_.Translation(attackCollider_->GetWorldPosition());
		attackColliderObj_->LocalToWorld();
		attackColliderObj_->SetWVPData(CameraSystem::GetInstance()->GetActiveCamera()->DrawCamera(attackColliderObj_->worldTransform_.mat_));
		attackColliderObj_->Draw();
	}*/
}

void Player::ChangeState(PlayerState* newState) {
	if (state_) {
		state_->Exit();

		delete state_;
		state_ = nullptr;
	}
	state_ = newState;
	state_->SetPlayer(this);
	state_->Initialize();
}

// ------------------------------
// AttackColliderの設定
// ------------------------------

void Player::EnableHitBox(bool enable, const Vector3& worldPos) {
	if (enable) {
		// 攻撃判定の位置を更新
		attackCollider_->SetWorldPosition(worldPos);

		// 攻撃判定をアクティブ化
		attackCollider_->SetMyType(COL_Player_Attack);

		// 相手のマスクも設定
		attackCollider_->SetYourType(COL_Enemy | COL_Enemy_Attack);
	}
	else {
		// 攻撃判定を非アクティブにする
		attackCollider_->SetMyType(COL_None);
		attackCollider_->SetYourType(COL_None);
	}

}

// ------------------------------
// 物理的な処理
// ------------------------------

void Player::ApplyPhysics() {
	if (isOnGround_ == false) {
		verticalVelocity_ -= gravity_;
	}

	// 一旦地面との当たり判定（地面を０とする）
	if (obj_->GetTransform().translate.y <= 0.0f) {
		isOnGround_ = true;
		Vector3 pos = obj_->GetTransform().translate;
		pos.y = 0.0f;
		obj_->SetTransform({ obj_->GetTransform().rotate,obj_->GetTransform().scale,pos });
	}

	if (isOnGround_ == true) {
		verticalVelocity_ = 0.0f;
	}
}

// ------------------------------
// スタミナについて
// ------------------------------

void Player::UpdateStaminaRecovery() {
	if (staminaRecoveryBlockers_ > 0) {
		// Blockerが１以上なら回復しない
		return;
	}

	// 回復処理
	stamina_ += staminaRecoveryRate_ * (1.0f / 60.0f);
	if (stamina_ > maxStamina_) {
		stamina_ = maxStamina_;
	}
}

// ------------------------------
// 旋回しょり
// ------------------------------

void Player::RotateToMoveDirection() {
	// 移動処理がなければスキップ
	if (move_.x == 0.0f && move_.z == 0.0f) {
		// 入力が無かった
		return;
	}

	float targetAngle = std::atan2(move_.x, move_.z);

	/*Quaternion targetQuaternion = Quaternion::MakeRotateAxisAngleQuaternion({ 0.0f,1.0f,0.0f }, targetAngle);

	Quaternion currentQuaternion = obj_->worldTransform_.get_.Quaternion();
	currentQuaternion = Quaternion::Slerp(currentQuaternion, targetQuaternion, rotateSpeed_);
	obj_->worldTransform_.set_.Quaternion(currentQuaternion);*/
}

Vector3 Player::GetForwardVector() {
	Vector3 ret;
	//ret = obj_->worldTransform_.get_.ForwardVector();
	return (ret);
}

// ---------------------------
// 攻撃を喰らった時の処理
// ---------------------------

void Player::TakeDamage(float damage) {
	// 1. 死亡状態、または無敵時間中の場合は処理をスキップ
	if (IsDead() || IsInvulnerable()) {
		return;
	}

	// 2. HPを減少させる (HPが0未満にならないように clamp する)
	hp_ -= damage;
	if (hp_ < 0.0f) {
		hp_ = 0.0f;
	}

	// 3. 死亡判定
	if (IsDead()) {
		// HPが0になったら死亡 State へ遷移
		ChangeState(new PlayerDeathState());

		// 本体Colliderを無効化（死亡後に当たり判定を残さないため）
		playerCollider_->SetMyType(COL_None);
	}
	else {
		// HPが残っている場合は被ダメージ State へ遷移
		ChangeState(new PlayerHurtState());

		// 無敵時間を設定する (HurtState側でリセットする)
		SetInvulnerable(true);
	}
}