#include "PCH.h"
#include "PlayerComponent.h"
#include "GameObject.h"
#include "ColliderComponent.h"
#include "ReticleComponent.h"

PlayerComponent::PlayerComponent()
	: movement_(std::make_unique<PlayerMovement>())
	, weapon_(std::make_unique<PlayerWeapon>())
	, turret_(std::make_unique<PlayerTurret>())
	, health_(std::make_unique<PlayerHealth>()) {
}

void PlayerComponent::Initialize() {
	if (isInitialized_) return;
	isInitialized_ = true;

	if (health_) health_->Initialize();
	if (movement_) movement_->Initialize();
	if (weapon_) weapon_->Initialize();
	if (turret_) turret_->Initialize();

	if (gameObject_) {
		gameObject_->GetTransform().translate = { 0.0f, 0.3f, 0.0f };

		// プレイヤーにコライダーがなければ自動追加
		auto* collider = gameObject_->GetComponent<ColliderComponent>();
		if (!collider) {
			collider = gameObject_->AddComponent<ColliderComponent>();
		}
		if (collider) {
			collider->SetRadius(2.0f); // 船のサイズに合わせた球判定
		}
	}
}

void PlayerComponent::Update() {
	if (isDebugMode_) {
		return;
	}

	// 体力・無敵・点滅演出更新
	if (health_) {
		health_->Update(gameObject_);
	}

	// 移動処理
	if (movement_) {
		movement_->Update(gameObject_);
	}

	// 射撃処理
	if (weapon_) {
		weapon_->Update(gameObject_, reticleObject_);
	}

	// 大砲（Canonノード）の追従回転処理
	if (turret_) {
		turret_->Update(gameObject_, reticleObject_);
	}
}

void PlayerComponent::TakeDamage(int damage) {
	if (health_) {
		health_->TakeDamage(gameObject_, damage);
	}
}

int PlayerComponent::GetHp() const {
	return health_ ? health_->GetHp() : 0;
}

int PlayerComponent::GetMaxHp() const {
	return health_ ? health_->GetMaxHp() : 0;
}

bool PlayerComponent::IsDead() const {
	return health_ ? health_->IsDead() : false;
}

const Vector3& PlayerComponent::GetForward() const {
	static const Vector3 s_defaultForward = { 0.0f, 0.0f, 1.0f };
	return movement_ ? movement_->GetForward() : s_defaultForward;
}

void PlayerComponent::TransitionToUnderwater() {
	if (movement_) {
		movement_->TransitionToUnderwater();
	}
}

bool PlayerComponent::IsUnderwater() const {
	return movement_ ? movement_->IsUnderwater() : false;
}

void PlayerComponent::ResolveReticle(const std::vector<std::unique_ptr<GameObject>>& gameObjects) {
	for (const auto& obj : gameObjects) {
		if (obj && obj->GetComponent<ReticleComponent>()) {
			reticleObject_ = obj.get();
			return;
		}
	}
}

void PlayerComponent::ImGui() {
#ifdef USEIMGUI
	if (health_) health_->ImGui();
	if (movement_) movement_->ImGui();
	if (weapon_) weapon_->ImGui();
	if (turret_) turret_->ImGui();
#endif
}

void PlayerComponent::Serialize(json& j) const {
	j["type"] = "PlayerComponent";
	if (health_) health_->Serialize(j);
	if (movement_) movement_->Serialize(j);
	if (weapon_) weapon_->Serialize(j);
	if (turret_) turret_->Serialize(j);
}

void PlayerComponent::Deserialize(const json& j) {
	isInitialized_ = true;
	if (health_) health_->Deserialize(j);
	if (movement_) movement_->Deserialize(j);
	if (weapon_) weapon_->Deserialize(j);
	if (turret_) turret_->Deserialize(j);
}
