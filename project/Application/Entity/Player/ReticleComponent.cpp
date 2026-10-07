#include "PCH.h"
#include "ReticleComponent.h"
#include "GameObject.h"
#include "MathFunction.h"
#include "CameraOrganizer.h"
#include "BaseScene.h"
#include "SpriteComponent.h"
#include "ColliderComponent.h"
#include "PlayerComponent.h"
#include "WindowsAPI.h"
#include "FishEnemyComponent.h"
#include "BirdEnemyComponent.h"
#include "Boss.h"
#include "FloatingCrateComponent.h"

void ReticleComponent::Initialize() {
	// すでに初期化済み（ロード済み）なら、デフォルト値での上書きをスキップする
	if(isInitialized_) return;
	isInitialized_ = true;
}

void ReticleComponent::Update() {
	if(!gameObject_ && !isDebugMode_) return;

	auto* context = gameObject_->GetContext();

	if(!isVisible_) {
		lockOnTarget_ = nullptr;
		// メッシュレンダラーを取得して透明にし非表示化
		if(auto* renderer = gameObject_->GetComponent<SpriteComponent>()) {
			renderer->SetColor({ 0.0f, 0.0f, 0.0f, 0.0f });
		}
		return; // 位置更新やロックオン判定をスキップ
	}

	// シングルトンのカメラオーガナイザーから現在のアクティブカメラの情報を取る
	CameraOrganizer* cameraOrganizer = CameraOrganizer::GetInstance();
	CameraData& cameraData = cameraOrganizer->GetCameraData();

	// カメラのワールド行列から「位置」と「前方ベクトル」を抽出する
	Vector3 camPos = { cameraData.world.m[3][0], cameraData.world.m[3][1], cameraData.world.m[3][2] };
	Vector3 camForward = { cameraData.world.m[2][0], cameraData.world.m[2][1], cameraData.world.m[2][2] };

	// プレイヤーの船の座標を取得（距離判定を船基準にするため）
	// プレイヤーのポインタと座標を用意
	GameObject* playerObj = nullptr;
	Vector3 playerPos = { 0.0f, 0.0f, 0.0f };
	bool foundPlayer = false;

	if (context && context->activeGameObjects) {
		for (const auto& obj : *(context->activeGameObjects)) {
			if (obj->GetName() == "Player" || obj->GetComponent<PlayerComponent>() != nullptr) {
				playerObj = obj.get();
				playerPos = obj->GetTransform().translate;
				foundPlayer = true;
				break;
			}
		}
	}

	// 毎フレームリセットしておく
	lockOnTarget_ = nullptr; 

	// 角度（内積）で判定する 1.0に近いほど画面中央。0.990f は画面中心から約8度以内の範囲
	float maxCos = 0.995f;

	if(context && context->activeGameObjects) {
		for(const auto& obj : *(context->activeGameObjects)) {
			// プレイヤー自身、弾、レティクル、カメラ以外の「コライダー付きオブジェクト」をすべてターゲットとみなす
			bool isEnemy = (obj->GetComponent<BirdEnemyComponent>() != nullptr || 
							obj->GetComponent<FishEnemyComponent>() != nullptr ||
							obj->GetComponent<BossComponent>() != nullptr ||
							obj->GetComponent<FloatingCrateComponent>() != nullptr ||
							obj->GetName() == "Enemy" ||
							obj->GetName() == "Boss" ||
							obj->GetName() == "Crate");

			if(isEnemy && obj->GetComponent<ColliderComponent>() != nullptr) {

				// 距離判定はプレイヤーからの距離で計算する
				Vector3 toEnemyFromPlayer = Math::Subtract(obj->GetTransform().translate, foundPlayer ? playerPos : camPos);
				float distToEnemy = Math::Length(toEnemyFromPlayer);

				// 向き（画面中央との一致度）はカメラの視線で計算
				Vector3 toEnemyFromCam = Math::Subtract(obj->GetTransform().translate, camPos);
				Vector3 dirToEnemy = Math::Normalize(toEnemyFromCam);
				float cosAngle = Math::Dot(dirToEnemy, camForward);

				// カメラの後ろにいる敵（内積0以下）は除外
				if (cosAngle <= 0.0f) continue;

				// プレイヤーの前方判定
				toEnemyFromPlayer = Math::Subtract(obj->GetTransform().translate, playerPos);
				Vector3 dirToEnemyFromPlayer = Math::Normalize(toEnemyFromPlayer);

				// プレイヤーの現在の正面向きベクトル
				float playerYaw = playerObj->GetTransform().rotate.y;
				Vector3 playerForward = { std::sin(playerYaw), 0.0f, std::cos(playerYaw) };

				float dotPlayer = Math::Dot(dirToEnemyFromPlayer, playerForward);

				// プレイヤーの船の背後にいる敵（内積0以下）も即除外！
				if (dotPlayer <= 0.0f) continue;

				// 距離と画面中央の最終チェック
				if (distToEnemy > 1.0f && distToEnemy <= lockOnMaxDistance_) {
					if (cosAngle > maxCos) {
						maxCos = cosAngle;
						lockOnTarget_ = obj.get();
					}
				}
			}
		}
	}

	// SpriteComponent の色を変更する
	if (auto* sprite = gameObject_->GetComponent<SpriteComponent>()) {
		// 位置ずれ解消
		gameObject_->GetTransform().translate = { 0.0f, 0.0f, 0.0f };

		float screenW = 1280.0f;
		float screenH = 720.0f;
		Vector2 finalPos = { screenW * 0.5f + spriteOffset_.x, screenH * 0.5f + spriteOffset_.y };

		sprite->SetAnchorPoint({ 0.5f, 0.5f });
		sprite->SetPosition(finalPos);
		sprite->SetSize(spriteSize_);
		sprite->SetScale(spriteScale_);           // スケール1倍

		if (lockOnTarget_) {
			// ロックオン時
			sprite->SetColor(lockOnColor_);
		} else {
			// 通常時
			sprite->SetColor(normalColor_);
		}
	}
}

void ReticleComponent::ImGui() {
	ImGui::ColorEdit4("Normal Color", &normalColor_.x);
	ImGui::ColorEdit4("LockOn Color", &lockOnColor_.x);
	ImGui::DragFloat2("Sprite Size", &spriteSize_.x, 1.0f, 1.0f, 500.0f);
	ImGui::DragFloat2("Sprite Offset", &spriteOffset_.x, 1.0f, -500.0f, 500.0f);
	ImGui::DragFloat2("Sprite Scale", &spriteScale_.x, 0.01f, 0.01f, 10.0f);
	ImGui::DragFloat("LockOn Sensitivity", &lockOnAngleCos_, 0.0001f, 0.9000f, 1.0000f, "%.4f");
	ImGui::DragFloat("LockOn Sensitivity", &lockOnAngleCos_, 0.0001f, 0.9000f, 1.0000f, "%.4f");
}

void ReticleComponent::Serialize(json& j) const {
	j["type"] = "ReticleComponent";
	j["NormalColor"] = { normalColor_.x, normalColor_.y, normalColor_.z, normalColor_.w };
	j["LockOnColor"] = { lockOnColor_.x, lockOnColor_.y, lockOnColor_.z, lockOnColor_.w };
	j["spriteSize"] = { spriteSize_.x, spriteSize_.y };
	j["spriteOffset"] = { spriteOffset_.x, spriteOffset_.y };
	j["spriteScale"] = { spriteScale_.x, spriteScale_.y };
	j["lockOnAngleCos"] = lockOnAngleCos_;
}

void ReticleComponent::Deserialize(const json& j) {
	isInitialized_ = true;
	if(j.contains("NormalColor")) {
		normalColor_ = { j["NormalColor"][0], j["NormalColor"][1], j["NormalColor"][2], j["NormalColor"][3] };
	}
	if(j.contains("LockOnColor")) {
		lockOnColor_ = { j["LockOnColor"][0], j["LockOnColor"][1], j["LockOnColor"][2], j["LockOnColor"][3] };
	}
	if (j.contains("spriteSize")) spriteSize_ = { j["spriteSize"][0], j["spriteSize"][1] };
	if (j.contains("spriteOffset")) spriteOffset_ = { j["spriteOffset"][0], j["spriteOffset"][1] };
	if (j.contains("spriteScale")) spriteScale_ = { j["spriteScale"][0], j["spriteScale"][1] };
	if(j.contains("lockOnAngleCos")) lockOnAngleCos_ = j["lockOnAngleCos"];
}