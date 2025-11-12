
#include "Player.h"
#include <algorithm>
#include <numbers>
#include <imgui.h>
#include "utility/input/InputManager.h"
#include "MagosuyaEngine.h"
#include "../scene/PlayScene.h"

//定数
const inline float deltaTime = 1.0f / 60.0f;
const inline float gravity = -1.0f;

Player::Player (MagosuyaEngine* magosuya, CameraData* cameraData) {
	magosuya_ = magosuya;
	cameraData_ = cameraData;
	model_ = std::make_unique<Model> (magosuya, "Resources/player", "player");
	weapon_ = std::make_unique<Model> (magosuya, "Resources/hammer", "hammer");
}

Player::~Player () {
	magosuya_->UnloadTexture ("Resources/player/Player.png");
}

void Player::Initialize () {
	magosuya_->LoadTexture ("Resources/player/Player.png", "player");
	magosuya_->LoadTexture ("Resources/hammer/hammer.png", "hammer");
	model_->Initialize ();
	weapon_->Initialize ();

	transform_ = model_->GetTransform ();
	transform_.rotate = { -std::numbers::pi_v<float> / 2.0f, 0.0f, std::numbers::pi_v<float> };
	transform_.translate = { -15.0f, -1.0f, 0.0f };
	transformWep_ = weapon_->GetTransform ();
	transformWep_.rotate = { -std::numbers::pi_v<float> / 2.0f, -std::numbers::pi_v<float> / 2.0f, 0.0f };

	velocity_ = { 0.0f, 0.0f, 0.0f };
	dir_ = Direction::Right;
	hp_ = 3;
	isInvincible_ = false;
	invincibleTime_ = 1.0f;
	isAir_ = false;
	airJump_ = false;
	isAvoid_ = false;
	avoidTimer_ = 0.2f;
	coolTime_ = false;
	avoidCoolTime_ = 0.8f;
	isAttack_ = false;
	attackTime_ = 0.1f;
}

void Player::Update () {
	move ();
	Attack ();

	model_->SetTransform (transform_);
	model_->Update (&cameraData_->View, &cameraData_->Proj);
	model_->SetColor (Vector4{ 1.0f, 1.0f, 1.0f, 1.0f });
	WeaponTransform ();
	weapon_->SetTransform (transformWep_);
	weapon_->Update (&cameraData_->View, &cameraData_->Proj);
	AABBPos ();
	Invincible ();
}

void Player::Draw () {
	model_->Draw (*magosuya_->GetTextureHandle ("player"));
	if (isAttack_) {
		weapon_->Draw (*magosuya_->GetTextureHandle ("hammer"));
	}
}

void Player::IsHit () {
	if (!isInvincible_) {
		hp_ -= 1;
		isInvincible_ = true;
	}
}

void Player::move () {
	if (g_inputManager->GetRawInput ()->Push ('A') && !isAvoid_) {
		dir_ = Direction::Left;
		velocity_.x = -0.2f;
	}
	if (g_inputManager->GetRawInput ()->Push ('D') && !isAvoid_) {
		dir_ = Direction::Right;
		velocity_.x = 0.2f;
	}
	if (!g_inputManager->GetRawInput ()->Push ('A') && !g_inputManager->GetRawInput ()->Push ('D')) {
		velocity_.x = 0.0f;
	}
	Jump ();
	Avoid ();

	transform_.translate.x += velocity_.x;
	if (!isAvoid_) {
		velocity_.y += gravity * deltaTime;
	}
	transform_.translate.y += velocity_.y;

	if (transform_.translate.y <= -1.0f) {
		transform_.translate.y = -1.0f;
		isAir_ = false;
		airJump_ = true;
	}
	else {
		isAir_ = true;
	}

	//移動制限
	if (transform_.translate.x <= -19.0f) {
		transform_.translate.x = -19.0f;
	}
	else if (transform_.translate.x >= 19.0f) {
		transform_.translate.x = 19.0f;
	}
}

void Player::Jump () {
	if (g_inputManager->GetRawInput ()->Trigger (VK_SPACE) && !isAir_) {
		velocity_.y = 0.4f;
	}
	if (g_inputManager->GetRawInput ()->Trigger (VK_SPACE) && isAir_ && airJump_) {
		velocity_.y = 0.3f;
		airJump_ = false;
	}
}

void Player::Avoid () {
	if (g_inputManager->GetRawInput ()->TriggerMouse (MouseButton::RIGHT) && !isAvoid_ && avoidCoolTime_ == 0.8f) {
		if (dir_ == Direction::Left) {
			velocity_.y = 0.0f;
			velocity_.x = -1.0f;
		}
		else if (dir_ == Direction::Right) {
			velocity_.y = 0.0f;
			velocity_.x = 1.0f;
		}
		isAvoid_ = true;
		coolTime_ = true;
	}
	if (isAvoid_) {
		avoidTimer_ -= deltaTime;
		if (velocity_.x >= 0.0f && dir_ == Direction::Right && avoidTimer_ >= 0.0f) {
			velocity_.x -= 0.1f;
		}
		if (velocity_.x <= 0.0f && dir_ == Direction::Left && avoidTimer_ >= 0.0f) {
			velocity_.x += 0.1f;
		}
		if (avoidTimer_ <= 0.0f) {
			avoidTimer_ = 0.2f;
			isAvoid_ = false;
		}
	}
	if (coolTime_) {
		avoidCoolTime_ -= deltaTime;
	}
	if (avoidCoolTime_ <= 0.0f) {
		avoidCoolTime_ = 0.8f;
		coolTime_ = false;
	}
}

void Player::Attack () {
	if (g_inputManager->GetRawInput ()->TriggerMouse (MouseButton::LEFT)) {
		isAttack_ = true;
		attackDir_ = dir_;
	}
	if (isAttack_) {
		attackTime_ -= deltaTime;
	}
	if (attackTime_ <= 0.0f) {
		isAttack_ = false;
		attackTime_ = 0.1f;
		attackIsHit_ = false;
	}
}

void Player::WeaponTransform () {
	Direction currentWeaponDir = dir_;
	if (isAttack_) {
		// 攻撃中は、攻撃開始時に記録した向きを強制的に使用する
		currentWeaponDir = attackDir_;
	}
	if (currentWeaponDir == Direction::Left) {
		transformWep_.translate.x = transform_.translate.x - 3.0f;
		transformWep_.rotate.y = std::numbers::pi_v<float> / 2.0f;
	}
	if (currentWeaponDir == Direction::Right) {
		transformWep_.translate.x = transform_.translate.x + 3.0f;
		transformWep_.rotate.y = -std::numbers::pi_v<float> / 2.0f;
	}
	transformWep_.translate.y = transform_.translate.y;
}

void Player::AABBPos () {
	aabb_model_.min = { transform_.translate.x - 1.0f,  transform_.translate.y - 1.0f, 0.0f };
	aabb_model_.max = { transform_.translate.x + 1.0f,  transform_.translate.y + 1.0f, 0.0f };
	aabb_weapon_.min = { transformWep_.translate.x - 2.0f,  transformWep_.translate.y - 2.0f, 0.0f };
	aabb_weapon_.max = { transformWep_.translate.x + 2.0f,  transformWep_.translate.y + 2.0f, 0.0f };
}

void Player::Invincible () {
	if (isInvincible_) {
		invincibleTime_ -= deltaTime;
		Vector4 color = { 1.0f, 1.0f, 1.0f, 0.5f };
		model_->SetColor (color);
	}
	if (invincibleTime_ <= 0.0f) {
		isInvincible_ = false;
		invincibleTime_ = 1.0f;
	}
}
