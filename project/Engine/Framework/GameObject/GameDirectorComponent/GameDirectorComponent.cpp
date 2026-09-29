#include "PCH.h"
#include "GameDirectorComponent.h"
#include "GameObject.h"
#include "BaseScene.h"
#include "NumberDrawerComponent.h"
#include "VirtualFollowCamera.h"
#include "PlayerComponent.h"
#include "EnemyManagerComponent.h"
#include "MeshRendererComponent.h"
#include "ColliderComponent.h"
#include "Boss.h"
#include "imgui.h"
#include "Logger.h" // ログ出力用（存在すれば）

void GameDirectorComponent::Initialize() {
	if (isInitialized_) return;
	isInitialized_ = true;

	UpdateUI();
}

void GameDirectorComponent::Update() {
	// 毎フレームUI更新を呼んでも安全（値が変わったときのみ更新などの最適化も可能だが、
	// エディタ等でパラメータを変えた時に即時反映させるため、ここでは毎フレーム更新を保証）
	UpdateUI();
}

void GameDirectorComponent::ImGui() {
#ifdef USEIMGUI
	ImGui::DragInt("Target Kills", &targetKills_, 1, 1, 1000);
	ImGui::DragInt("Current Kills", &currentKills_, 1, 0, targetKills_);

	char nameBuf[128];
	strcpy_s(nameBuf, uiObjectName_.c_str());
	if (ImGui::InputText("UI Object Name", nameBuf, sizeof(nameBuf))) {
		uiObjectName_ = nameBuf;
	}

	ImGui::Text("Boss Event Triggered: %s", isBossEventTriggered_ ? "TRUE" : "FALSE");

	if (ImGui::Button("Manual Trigger Event")) {
		OnTargetKillsAchieved();
	}

	if (ImGui::Button("Reset Director")) {
		currentKills_ = 0;
		isBossEventTriggered_ = false;
		UpdateUI();

		// 水上モードにリセット
		if (gameObject_ && gameObject_->GetContext() && gameObject_->GetContext()->activeGameObjects) {
			for (auto& obj : *(gameObject_->GetContext()->activeGameObjects)) {
				if (auto* player = obj->GetComponent<PlayerComponent>()) {
					if (auto* move = player->GetMovement()) {
						move->SetUnderwater(false);
						obj->GetTransform().translate.y = 0.3f;
					}
				}
				if (auto* followCam = obj->GetComponent<VirtualFollowCamera>()) {
					followCam->SetUnderwater(false);
				}
				if (auto* enemyMgr = obj->GetComponent<EnemyManagerComponent>()) {
					enemyMgr->SetSpawningEnabled(true);
				}
				if (obj->GetName() == "Boss" || obj->GetComponent<BossComponent>() != nullptr) {
					obj->Destroy();
				}
			}
		}
	}
#endif
}

void GameDirectorComponent::Serialize(json& j) const {
	j["type"] = "GameDirectorComponent";
	j["targetKills"] = targetKills_;
	j["currentKills"] = currentKills_;
	j["isBossEventTriggered"] = isBossEventTriggered_;
	j["uiObjectName"] = uiObjectName_;
}

void GameDirectorComponent::Deserialize(const json& j) {
	isInitialized_ = true;
	if (j.contains("targetKills")) targetKills_ = j["targetKills"];
	if (j.contains("currentKills")) currentKills_ = j["currentKills"];
	if (j.contains("isBossEventTriggered")) isBossEventTriggered_ = j["isBossEventTriggered"];
	if (j.contains("uiObjectName")) uiObjectName_ = j["uiObjectName"];
}

void GameDirectorComponent::NotifyEnemyDead() {
	if (isBossEventTriggered_) return;

	currentKills_++;
	UpdateUI();

	if (currentKills_ >= targetKills_) {
		isBossEventTriggered_ = true;
		OnTargetKillsAchieved();
	}
}

void GameDirectorComponent::OnTargetKillsAchieved() {
	// 目標撃破数を達成したときの処理！
	// 水上から水中フェーズ（ボスフェーズ）へ移行
#ifdef _DEBUG
	OutputDebugStringA("--- Game Director: Target Kills Achieved! Transitioning to Underwater Phase ---\n");
#endif

	isBossEventTriggered_ = true;

	if (gameObject_) {
		SceneContext* context = gameObject_->GetContext();
		if (context && context->activeGameObjects) {
			Vector3 playerPos = { 0.0f, -30.0f, 0.0f };
			float playerYaw = 0.0f;

			for (auto& obj : *(context->activeGameObjects)) {
				// 1. プレイヤーを水中へ潜航させる
				if (auto* player = obj->GetComponent<PlayerComponent>()) {
					player->TransitionToUnderwater();
					playerPos = obj->GetTransform().translate;
					playerYaw = obj->GetTransform().rotate.y;
				}

				// 2. 追従カメラを水中モードに切り替える
				if (auto* followCam = obj->GetComponent<VirtualFollowCamera>()) {
					followCam->SetUnderwater(true);
				}

				// 3. 雑魚敵マネージャーの新規スポーンを停止し、水上の敵を一掃
				if (auto* enemyMgr = obj->GetComponent<EnemyManagerComponent>()) {
					enemyMgr->SetSpawningEnabled(false);
					enemyMgr->ClearAllEnemies();
				}
			}

			// 4. ボスの生成（既に存在していないか確認）
			bool bossExists = false;
			for (const auto& obj : *(context->activeGameObjects)) {
				if (obj->GetName() == "Boss" || obj->GetComponent<BossComponent>() != nullptr) {
					bossExists = true;
					break;
				}
			}

			if (!bossExists && context->gameObjects) {
				auto bossObj = std::make_unique<GameObject>(context, "Boss");

				// プレイヤーの前方約45m、水深-35mに出現
				float spawnDist = 45.0f;
				Vector3 spawnPos = playerPos;
				spawnPos.x += std::sin(playerYaw) * spawnDist;
				spawnPos.z += std::cos(playerYaw) * spawnDist;
				spawnPos.y = -35.0f;

				bossObj->GetTransform().translate = spawnPos;
				bossObj->GetTransform().scale = { 8.0f, 8.0f, 8.0f };
				bossObj->GetTransform().rotate.y = playerYaw + 3.14159265f; // プレイヤーの方を向く

				// レンダラー設定（巨大深海魚モデル）
				auto* mesh = bossObj->AddComponent<MeshRendererComponent>();
				mesh->SetModel("Resources/Enemy/smallFish/smallFish.obj");
				mesh->SetTexture("white1x1");
				mesh->SetColor({ 0.6f, 0.7f, 0.95f, 1.0f });

				// ボス挙動・当たり判定
				bossObj->AddComponent<BossComponent>();
				auto* collider = bossObj->AddComponent<ColliderComponent>();

				bossObj->Initialize();
				collider->SetRadius(8.0f);

				bossObj->SetSerializable(false);

				context->gameObjects->push_back(std::move(bossObj));
			}
		}
	}
}

void GameDirectorComponent::UpdateUI() {
	if (!gameObject_) return;
	SceneContext* context = gameObject_->GetContext();
	if (!context || !context->activeGameObjects) return;

	// UIオブジェクトを検索
	for (auto& obj : *(context->activeGameObjects)) {
		if (obj->GetName() == uiObjectName_) {
			if (auto* drawer = obj->GetComponent<NumberDrawerComponent>()) {
				int remaining = std::max(0, targetKills_ - currentKills_);
				drawer->SetValue(remaining);
				break;
			}
		}
	}
}
