#include "Player.h"
#include "MathFunction.h"
#include "imgui.h"

Player::~Player() {
	delete state_;
}

void Player::Initialize()
{
	obj_ = std::make_unique<Model>(engine_);
	obj_->SetModelData("teapot");
	obj_->SetTexture("teapot");
	obj_->Initialize();

	// 始まりのState
	state_ = new PlayerStopState();
	state_->SetPlayer(this);

	attackColliderObj_ = std::make_unique<Model>(engine_);
	attackColliderObj_->SetModelData("teapot");
	attackColliderObj_->SetTexture("teapot");
	attackColliderObj_->Initialize();

	// Colliderの設定
	// 1, Player
	playerCollider_ = std::make_unique<PlayerBodyCollider>(this);
	// 2, Attack
	attackCollider_ = std::make_unique<AttackCollider>();
	EnableHitBox(false, obj_->GetTransform().translate);
}

void Player::Update(Matrix4x4* m)
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
	if (move_.x != 0.0f || move_.y != 0.0f || move_.z != 0.0f) {
		move_ = Normalize(move_);
		direction_.x = move_.x;
		direction_.z = move_.z;
	}
	move_.y = 1.0f;

	RotateToMoveDirection();

	Vector3 pos = obj_->GetTransform().translate;
	pos.x += move_.x * speed_ * speedMultiplier_;
	pos.y += move_.y * verticalVelocity_;
	pos.z += move_.z * speed_ * speedMultiplier_;
	obj_->SetTransform({ obj_->GetTransform().scale,obj_->GetTransform().rotate,pos});

	attackColliderObj_->SetTransform({ {attackCollider_->GetRadius(),0.1f,attackCollider_->GetRadius()},{0.0f,0.0f,0.0f},
		{attackCollider_->GetWorldPosition().x,
		attackCollider_->GetWorldPosition().y - 0.5f,
		attackCollider_->GetWorldPosition().z} });

	obj_->Update(m);
	attackColliderObj_->Update(m);
#ifdef _DEBUG
	ImGui::Begin("Player");
	ImGui::SliderFloat3("Direction", &direction_.x,0.0f,0.0f);
	ImGui::End();
	obj_->ImGui();
#endif//_DEBUG
}

void Player::Draw()
{
	obj_->Draw();

	if (isAttackViewFlag_) {
		attackColliderObj_->Draw();
	}
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
		Matrix4x4 w = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, worldPos);
		Matrix4x4 pW = MakeAffineMatrix({ obj_->GetTransform().scale }, { obj_->GetTransform().rotate }, { obj_->GetTransform().translate });
		w = w * pW;

		attackCollider_->SetWorldPosition({w.m[3][0],w.m[3][1] ,w.m[3][2] });

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
		obj_->SetTransform({ obj_->GetTransform().scale,obj_->GetTransform().rotate,pos });
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

	targetAngle = std::fmod(targetAngle, Deg2Rad(360));

	if (targetAngle >= Deg2Rad(180)) {
		targetAngle -= Deg2Rad(360);
	}
	else if (targetAngle <= Deg2Rad(-180)) {
		targetAngle += Deg2Rad(360);
	}


	Vector3 rot = obj_->GetTransform().rotate;
	rot.y = 0.0f * obj_->GetTransform().rotate.y + (1.0f - 0.0f) * targetAngle;
	obj_->SetTransform({
		obj_->GetTransform().scale,
		rot,
		obj_->GetTransform().translate
		});
	/*Quaternion targetQuaternion = Quaternion::MakeRotateAxisAngleQuaternion({ 0.0f,1.0f,0.0f }, targetAngle);

	Quaternion currentQuaternion = obj_->worldTransform_.get_.Quaternion();
	currentQuaternion = Quaternion::Slerp(currentQuaternion, targetQuaternion, rotateSpeed_);
	obj_->worldTransform_.set_.Quaternion(currentQuaternion);*/
}

Vector3 Player::GetForwardVector() {
	return (direction_);
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