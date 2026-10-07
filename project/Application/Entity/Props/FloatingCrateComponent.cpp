#include "PCH.h"
#include "FloatingCrateComponent.h"
#include "EnergyItemComponent.h"
#include "GameObject.h"
#include "MeshRendererComponent.h"
#include "ColliderComponent.h"
#include "BaseScene.h"
#include "Time.h"
#include "MathFunction.h"
#include "../../../../Engine/Editor/ParticleEditor/ParticleSpawner.h"
#include <cmath>

void FloatingCrateComponent::Initialize() {
	if (!gameObject_) return;

	// 木箱の色（茶色）を設定
	if (auto* mesh = gameObject_->GetComponent<MeshRendererComponent>()) {
		mesh->SetColor({ 0.65f, 0.45f, 0.25f, 1.0f });
	}

	waterSurfaceY_ = gameObject_->GetTransform().translate.y;
	bobbingTimer_ = static_cast<float>(rand() % 100) * 0.1f; // ランダム位相
}

void FloatingCrateComponent::Update() {
	if (!gameObject_ || isDestroyed_) return;

	float dt = Time::GetDeltaTime();

	// ライフタイムチェック
	lifeTimer_ += dt;
	if (lifeTimer_ >= lifetime_) {
		gameObject_->Destroy();
		return;
	}

	auto& trans = gameObject_->GetTransform();

	// 水流による緩やかな漂流移動
	trans.translate.x += driftVelocity_.x * dt;
	trans.translate.z += driftVelocity_.z * dt;

	// 水面での上下揺れ（ボビング）
	bobbingTimer_ += bobbingSpeed_ * dt;
	trans.translate.y = waterSurfaceY_ + std::sin(bobbingTimer_) * bobbingAmount_;

	// わずかな自転と傾き揺れ
	trans.rotate.y += 0.2f * dt;
	trans.rotate.x = std::sin(bobbingTimer_ * 0.7f) * 0.08f;
	trans.rotate.z = std::cos(bobbingTimer_ * 0.8f) * 0.08f;
}

void FloatingCrateComponent::TakeDamage(int damage) {
	if (isDestroyed_) return;

	hp_ -= damage;
	if (hp_ <= 0) {
		hp_ = 0;
		OnDestroyed();
	}
}

void FloatingCrateComponent::OnDestroyed() {
	if (isDestroyed_) return;
	isDestroyed_ = true;

	if (!gameObject_) return;
	auto* context = gameObject_->GetContext();
	if (!context || !context->gameObjects) return;

	Vector3 spawnPos = gameObject_->GetTransform().translate;

	// 木片爆発パーティクル生成
	ParticleSpawner::SpawnExplosion(context, spawnPos, 15);

	// 中身（電力バッテリーアイテム）の生成
	auto itemObj = std::make_unique<GameObject>(context, "EnergyItem");
	auto* mesh = itemObj->AddComponent<MeshRendererComponent>();
	mesh->SetModel("Resources/Props/Crate/crate.obj");
	mesh->SetTexture("white1x1");
	mesh->SetColor({ 0.2f, 1.0f, 0.85f, 1.0f }); // ピカピカ光るシアン色

	auto* collider = itemObj->AddComponent<ColliderComponent>();
	collider->SetRadius(2.2f); // 拾いやすい当たり判定

	itemObj->AddComponent<EnergyItemComponent>();

	itemObj->GetTransform().translate = spawnPos;
	itemObj->GetTransform().translate.y = waterSurfaceY_ + 0.3f; // 水面にポンと出現
	itemObj->GetTransform().scale = { 0.45f, 0.45f, 0.45f };    // 小型バッテリーサイズ

	itemObj->Initialize();
	itemObj->SetSerializable(false);

	context->gameObjects->push_back(std::move(itemObj));

	// 木箱自身を破棄
	gameObject_->Destroy();
}

void FloatingCrateComponent::ImGui() {
#ifdef USEIMGUI
	ImGui::Text("--- Floating Crate ---");
	ImGui::DragInt("HP", &hp_, 1, 1, 10);
	ImGui::DragFloat("Water Surface Y", &waterSurfaceY_, 0.1f, -10.0f, 10.0f);
	ImGui::DragFloat("Bobbing Speed", &bobbingSpeed_, 0.1f, 0.1f, 10.0f);
	ImGui::DragFloat("Bobbing Amount", &bobbingAmount_, 0.05f, 0.0f, 2.0f);
	ImGui::DragFloat3("Drift Velocity", &driftVelocity_.x, 0.05f, -5.0f, 5.0f);
	ImGui::DragFloat("Lifetime", &lifetime_, 1.0f, 10.0f, 300.0f);
	if (ImGui::Button("Destroy (Drop Item)")) {
		OnDestroyed();
	}
#endif
}

void FloatingCrateComponent::Serialize(json& j) const {
	j["type"] = "FloatingCrateComponent";
	j["hp"] = hp_;
	j["waterSurfaceY"] = waterSurfaceY_;
	j["bobbingSpeed"] = bobbingSpeed_;
	j["bobbingAmount"] = bobbingAmount_;
	j["driftVelocity"] = { driftVelocity_.x, driftVelocity_.y, driftVelocity_.z };
	j["lifetime"] = lifetime_;
}

void FloatingCrateComponent::Deserialize(const json& j) {
	if (j.contains("hp")) hp_ = j["hp"];
	if (j.contains("waterSurfaceY")) waterSurfaceY_ = j["waterSurfaceY"];
	if (j.contains("bobbingSpeed")) bobbingSpeed_ = j["bobbingSpeed"];
	if (j.contains("bobbingAmount")) bobbingAmount_ = j["bobbingAmount"];
	if (j.contains("driftVelocity")) {
		driftVelocity_ = { j["driftVelocity"][0], j["driftVelocity"][1], j["driftVelocity"][2] };
	}
	if (j.contains("lifetime")) lifetime_ = j["lifetime"];
}
