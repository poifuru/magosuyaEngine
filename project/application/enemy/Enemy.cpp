#include "Enemy.h"
#include <algorithm>
#include <numbers>
#include "../../externals/imgui/imgui.h"
#include "../../magosuya/utility/input/InputManager.h"
#include "../../magosuya/MagosuyaEngine.h"
#include "../scene/PlayScene.h"

Enemy::Enemy (MagosuyaEngine* magosuya, CameraData* cameraData) {
	magosuya_ = magosuya;
	cameraData_ = cameraData;
	model_ = std::make_unique<Model> (magosuya, "Resources/player", "player");
}

Enemy::~Enemy () {
	magosuya_->UnloadTexture ("Resources/player/Player.png");
}

void Enemy::Initialize () {
	model_->Initialize ();
	transform_.scale = { 3.0f, 3.0f, 3.0f };
	transform_.rotate = { -std::numbers::pi_v<float> / 2.0f, 0.0f, std::numbers::pi_v<float> };
	transform_.translate = { 10.0f, 2.0f, 0.0f };
	hp_ = 20;
	isHit_ = false;
}

void Enemy::Update () {
	model_->SetTransform (transform_);
	model_->Update (&cameraData_->View, &cameraData_->Proj);
	model_->SetColor (Vector4{ 1.0f, 1.0f, 1.0f, 1.0f });
	AABBPos ();
}

void Enemy::Draw () {
	model_->Draw (*magosuya_->GetTextureHandle ("player"));
}

void Enemy::IsHit (bool attackIsHit) {
	if (attackIsHit) {
		hp_ -= 1;
		Vector4 color = { 1.0f, 0.0f, 0.0f, 1.0f };
		model_->SetColor (color);
		attackIsHit = false;
	}
}

void Enemy::AABBPos () {
	aabb_model_.min = { transform_.translate.x - 3.0f,  transform_.translate.y - 3.0f, 0.0f };
	aabb_model_.max = { transform_.translate.x + 3.0f,  transform_.translate.y + 3.0f, 0.0f };
}