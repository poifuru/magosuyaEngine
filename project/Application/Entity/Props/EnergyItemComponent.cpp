#include "PCH.h"
#include "EnergyItemComponent.h"
#include "PlayerComponent.h"
#include "GameObject.h"
#include "MeshRendererComponent.h"
#include "BaseScene.h"
#include "Time.h"
#include "MathFunction.h"
#include "../../../../Engine/Editor/ParticleEditor/ParticleSpawner.h"
#include <cmath>

void EnergyItemComponent::Initialize() {
	if (!gameObject_) return;
	waterSurfaceY_ = gameObject_->GetTransform().translate.y;
}

void EnergyItemComponent::Update() {
	if (!gameObject_ || isCollected_) return;

	float dt = Time::GetDeltaTime();

	// ライフタイムチェック
	lifeTimer_ += dt;
	if (lifeTimer_ >= lifetime_) {
		gameObject_->Destroy();
		return;
	}

	floatTimer_ += dt;

	auto& trans = gameObject_->GetTransform();

	// 水面での上下バウンド＆回転
	trans.translate.y = waterSurfaceY_ + std::sin(floatTimer_ * 3.0f) * 0.2f;
	trans.rotate.y += 2.5f * dt;
	trans.rotate.x = std::sin(floatTimer_ * 2.0f) * 0.15f;

	// 発光パルス演出（明るいシアン〜エメラルドグリーンに脈動）
	if (auto* mesh = gameObject_->GetComponent<MeshRendererComponent>()) {
		float pulse = 0.7f + 0.3f * std::sin(floatTimer_ * 6.0f);
		mesh->SetColor({ 0.2f * pulse, 1.0f * pulse, 0.85f * pulse, 1.0f });
	}

	// プレイヤーとの距離判定（近接自動回収ガード）
	if (gameObject_->GetContext() && gameObject_->GetContext()->activeGameObjects) {
		for (auto& obj : *(gameObject_->GetContext()->activeGameObjects)) {
			if (auto* playerComp = obj->GetComponent<PlayerComponent>()) {
				Vector3 diff = Math::Subtract(obj->GetTransform().translate, trans.translate);
				// プレイヤー潜水艦の半径を考慮して3.5m以内で回収成立
				if (Math::Length(diff) < 3.5f) {
					OnCollect(playerComp);
					return;
				}
			}
		}
	}
}

void EnergyItemComponent::OnCollect(PlayerComponent* player) {
	if (isCollected_) return;
	isCollected_ = true;

	if (player) {
		player->Heal(healAmount_);
	}

	if (gameObject_) {
		// 回収エフェクト（キラキラ爆発パーティクル）
		if (gameObject_->GetContext()) {
			ParticleSpawner::SpawnExplosion(gameObject_->GetContext(), gameObject_->GetTransform().translate, 20);
		}
		gameObject_->Destroy();
	}
}

void EnergyItemComponent::ImGui() {
#ifdef USEIMGUI
	ImGui::Text("--- Energy Battery Item ---");
	ImGui::DragInt("Heal Amount (Power)", &healAmount_, 1, 1, 100);
	ImGui::DragFloat("Water Surface Y", &waterSurfaceY_, 0.1f, -10.0f, 10.0f);
	ImGui::DragFloat("Lifetime", &lifetime_, 1.0f, 10.0f, 300.0f);
#endif
}

void EnergyItemComponent::Serialize(json& j) const {
	j["type"] = "EnergyItemComponent";
	j["healAmount"] = healAmount_;
	j["waterSurfaceY"] = waterSurfaceY_;
	j["lifetime"] = lifetime_;
}

void EnergyItemComponent::Deserialize(const json& j) {
	if (j.contains("healAmount")) healAmount_ = j["healAmount"];
	if (j.contains("waterSurfaceY")) waterSurfaceY_ = j["waterSurfaceY"];
	if (j.contains("lifetime")) lifetime_ = j["lifetime"];
}
