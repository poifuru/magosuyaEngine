#include "PCH.h"
#include "EnemyManagerComponent.h"
#include "GameObject.h"
#include "BirdEnemyComponent.h"
#include "FishEnemyComponent.h"
#include "ColliderComponent.h"
#include "BaseScene.h" // SceneContext や gameObjects へのアクセス用
#include "PlayerComponent.h" // プレイヤー検索用
#include "MeshRendererComponent.h"
#include "FloatingCrateComponent.h"
#include <cmath>

void EnemyManagerComponent::Initialize() {
	if (isInitialized_) return;

	isInitialized_ = true;
	// タイマーの初期化
	spawnTimer_ = spawnInterval_;
	crateSpawnTimer_ = 5.0f; // 初回は5秒後に出現
}

void EnemyManagerComponent::Update() {
	if (!gameObject_) return;

	// 縄張りで旋回中（または攻撃行動中）の鳥エネミーを集めて、隊列（フォーメーション）情報を自動配信する
	auto* context = gameObject_->GetContext();
	if (context && context->activeGameObjects) {
		std::vector<BirdEnemyComponent*> activeBirds;
		for (const auto& obj : *(context->activeGameObjects)) {
			if (auto* bird = obj->GetComponent<BirdEnemyComponent>()) {
				if (!bird->IsDead() && bird->GetState() != BirdState::Patrol) {
					activeBirds.push_back(bird);
				}
			}
		}

		int totalBirds = static_cast<int>(activeBirds.size());
		for (int i = 0; i < totalBirds; ++i) {
			activeBirds[i]->SetFormationInfo(i, totalBirds);
		}
	}

	float dt = Time::GetDeltaTime();

	// 敵のスポーンタイマー
	if (isSpawningEnabled_) {
		spawnTimer_ -= dt;
		if (spawnTimer_ <= 0.0f) {
			spawnTimer_ = spawnInterval_;
			SpawnEnemy();
		}
	}

	// 木箱のスポーンタイマー
	if (isCrateSpawningEnabled_) {
		crateSpawnTimer_ -= dt;
		if (crateSpawnTimer_ <= 0.0f) {
			crateSpawnTimer_ = crateSpawnInterval_;
			SpawnCrate();
		}
	}
}

void EnemyManagerComponent::ClearAllEnemies() {
	if (!gameObject_) return;
	auto* context = gameObject_->GetContext();
	if (!context || !context->activeGameObjects) return;

	for (const auto& obj : *(context->activeGameObjects)) {
		if (auto* bird = obj->GetComponent<BirdEnemyComponent>()) {
			bird->OnDead();
		}
		if (auto* fish = obj->GetComponent<FishEnemyComponent>()) {
			fish->OnDead();
		}
	}
}

void EnemyManagerComponent::SpawnEnemy() {
	auto* context = gameObject_->GetContext();

	if (!context || !context->gameObjects || !context->activeGameObjects) return;

	// 現在の生存敵数をカウント
	int currentEnemyCount = 0;
	for (const auto& obj : *(context->activeGameObjects)) {
		if (obj->GetComponent<BirdEnemyComponent>() != nullptr || obj->GetComponent<FishEnemyComponent>() != nullptr) {
			currentEnemyCount++;
		}
	}

	// 最大数を超えていたら新規スポーンしない
	if (currentEnemyCount >= maxEnemies_) return;

	// 基準となるプレイヤーの位置を探す
	Vector3 playerPos = { 0.0f, 0.0f, 0.0f };
	bool foundPlayer = false;

	for (const auto& obj : *(context->activeGameObjects)) {
		if (obj->GetName() == "Player" || obj->GetComponent<PlayerComponent>() != nullptr) {
			playerPos = obj->GetTransform().translate;
			foundPlayer = true;

			break;
		}
	}

	// プレイヤーが見つからない場合はスポーンさせない
	if (!foundPlayer) return;

	// プレイヤーの周囲のランダムな位置をスポーン座標にする
	float angle = static_cast<float>(rand()) / RAND_MAX * 3.14159265f * 2.0f;
	Vector3 spawnPos = {
		playerPos.x + spawnRadius_ * std::cos(angle),
		playerPos.y, // 高さはプレイヤーと同じ
		playerPos.z + spawnRadius_ * std::sin(angle)
	};

	// 敵のタイプをランダムで決定 (0: 鳥, 1: 魚)
	int enemyType = rand() % 2;
	auto enemyObj = std::make_unique<GameObject>(context, "Enemy");

	if (enemyType == 0) {
		// --- 鳥エネミーの生成 ---
		auto* mesh = enemyObj->AddComponent<MeshRendererComponent>();
		mesh->SetModel("Resources/Enemy/Bird/bird.obj");
		mesh->SetTexture("white1x1");
		// スケール
		enemyObj->GetTransform().scale = { 0.2f, 0.2f, 0.2f };
		// 挙動とコライダーを追加
		enemyObj->AddComponent<BirdEnemyComponent>();
		auto* collider = enemyObj->AddComponent<ColliderComponent>();
		// 初期座標を適用
		enemyObj->GetTransform().translate = spawnPos;
		enemyObj->GetTransform().translate.y = spawnPos.y + 4.0f;	// y座標をちょっと高めに
		// コンポーネントをすべて追加した後に初期化を呼ぶ
		enemyObj->Initialize();
		// 初期化完了後にコライダーの半径を設定（デフォルト値を上書き）
		collider->SetRadius(1.0f);
	}
	else {
		// --- 魚エネミーの生成 ---
		auto* mesh = enemyObj->AddComponent<MeshRendererComponent>();
		mesh->SetModel("Resources/Enemy/smallFish/smallFish.obj");
		mesh->SetTexture("white1x1");
		// スケール
		enemyObj->GetTransform().scale = { fishScale_, fishScale_, fishScale_ };
		// 挙動とコライダーを追加
		enemyObj->AddComponent<FishEnemyComponent>();
		auto* collider = enemyObj->AddComponent<ColliderComponent>();
		// 初期座標を適用
		enemyObj->GetTransform().translate = spawnPos;
		// コンポーネントをすべて追加した後に初期化を呼ぶ
		enemyObj->Initialize();
		// 初期化完了後にコライダーの半径を設定
		collider->SetRadius(fishColliderRadius_);
	}
	// 動的生成なのでセーブ対象外に
	enemyObj->SetSerializable(false);

	// シーンのオブジェクトリストに追加
	context->gameObjects->push_back(std::move(enemyObj));
}

void EnemyManagerComponent::SpawnCrate() {
	auto* context = gameObject_->GetContext();
	if (!context || !context->gameObjects || !context->activeGameObjects) return;

	// 現在の生存木箱数をカウント
	int currentCrates = 0;
	for (const auto& obj : *(context->activeGameObjects)) {
		if (obj->GetComponent<FloatingCrateComponent>() != nullptr) {
			currentCrates++;
		}
	}
	if (currentCrates >= maxCrates_) return;

	// プレイヤーの位置と向きを取得
	Vector3 playerPos = { 0.0f, 0.0f, 0.0f };
	Vector3 playerFwd = { 0.0f, 0.0f, 1.0f };
	bool foundPlayer = false;
	for (const auto& obj : *(context->activeGameObjects)) {
		if (auto* playerComp = obj->GetComponent<PlayerComponent>()) {
			playerPos = obj->GetTransform().translate;
			playerFwd = playerComp->GetForward();
			foundPlayer = true;
			break;
		}
	}
	if (!foundPlayer) return;

	// プレイヤーの前方扇状範囲（-60度〜+60度）にスポーン
	float randomDeg = static_cast<float>(rand() % 120) - 60.0f;
	float randomRad = randomDeg * (3.14159265f / 180.0f);
	float baseAngle = std::atan2(playerFwd.x, playerFwd.z);
	float spawnAngle = baseAngle + randomRad;
	float dist = crateSpawnRadius_ + static_cast<float>(rand() % 16) - 8.0f;

	Vector3 spawnPos = {
		playerPos.x + dist * std::sin(spawnAngle),
		0.0f, // 水面
		playerPos.z + dist * std::cos(spawnAngle)
	};

	auto crateObj = std::make_unique<GameObject>(context, "Crate");
	auto* mesh = crateObj->AddComponent<MeshRendererComponent>();
	mesh->SetModel("Resources/Props/Crate/crate.obj");
	mesh->SetTexture("white1x1");
	mesh->SetColor({ 0.65f, 0.45f, 0.25f, 1.0f }); // 木の色

	crateObj->GetTransform().scale = { 1.2f, 1.2f, 1.2f }; // 視認しやすいサイズ
	crateObj->GetTransform().translate = spawnPos;

	crateObj->AddComponent<FloatingCrateComponent>();
	auto* collider = crateObj->AddComponent<ColliderComponent>();

	crateObj->Initialize();
	collider->SetRadius(1.8f); // 弾が当たりやすい判定
	crateObj->SetSerializable(false);

	context->gameObjects->push_back(std::move(crateObj));
}

void EnemyManagerComponent::ClearAllCrates() {
	if (!gameObject_) return;
	auto* context = gameObject_->GetContext();
	if (!context || !context->activeGameObjects) return;

	for (const auto& obj : *(context->activeGameObjects)) {
		if (auto* crate = obj->GetComponent<FloatingCrateComponent>()) {
			crate->OnDestroyed();
		}
	}
}

void EnemyManagerComponent::ImGui() {
	ImGui::Text("--- Enemy Spawning ---");
	ImGui::Checkbox("Spawning Enabled", &isSpawningEnabled_);
	if (ImGui::Button("Clear All Enemies")) {
		ClearAllEnemies();
	}
	ImGui::DragInt("Max Enemies", &maxEnemies_, 1, 1, 100);
	ImGui::DragFloat("Spawn Interval", &spawnInterval_, 0.1f, 0.5f, 60.0f);
	ImGui::DragFloat("Spawn Radius", &spawnRadius_, 0.5f, 5.0f, 100.0f);
	ImGui::DragFloat("Fish Scale", &fishScale_, 0.1f, 0.5f, 10.0f);
	ImGui::DragFloat("Fish Collider Radius", &fishColliderRadius_, 0.1f, 0.5f, 10.0f);

	// 現在の生存敵数を計算して表示
	int currentEnemyCount = 0;
	int currentCrateCount = 0;
	if (gameObject_ && gameObject_->GetContext() && gameObject_->GetContext()->activeGameObjects) {
		for (const auto& obj : *(gameObject_->GetContext()->activeGameObjects)) {
			if (obj->GetComponent<BirdEnemyComponent>() != nullptr || obj->GetComponent<FishEnemyComponent>() != nullptr) {
				currentEnemyCount++;
			}
			if (obj->GetComponent<FloatingCrateComponent>() != nullptr) {
				currentCrateCount++;
			}
		}
	}
	ImGui::Text("Current Enemies: %d / %d", currentEnemyCount, maxEnemies_);

	ImGui::Separator();
	ImGui::Text("--- Floating Crate Spawning ---");
	ImGui::Checkbox("Crate Spawning Enabled", &isCrateSpawningEnabled_);
	if (ImGui::Button("Destroy All Crates")) {
		ClearAllCrates();
	}
	ImGui::DragInt("Max Crates", &maxCrates_, 1, 1, 20);
	ImGui::DragFloat("Crate Spawn Interval", &crateSpawnInterval_, 0.5f, 1.0f, 60.0f);
	ImGui::DragFloat("Crate Spawn Radius", &crateSpawnRadius_, 1.0f, 5.0f, 150.0f);
	ImGui::Text("Current Crates: %d / %d", currentCrateCount, maxCrates_);
}

void EnemyManagerComponent::Serialize(json& j) const {
	j["type"] = "EnemyManagerComponent";
	j["maxEnemies"] = maxEnemies_;
	j["spawnInterval"] = spawnInterval_;
	j["spawnRadius"] = spawnRadius_;
	j["isSpawningEnabled"] = isSpawningEnabled_;
	j["fishScale"] = fishScale_;
	j["fishColliderRadius"] = fishColliderRadius_;
	j["isCrateSpawningEnabled"] = isCrateSpawningEnabled_;
	j["maxCrates"] = maxCrates_;
	j["crateSpawnInterval"] = crateSpawnInterval_;
	j["crateSpawnRadius"] = crateSpawnRadius_;
}

void EnemyManagerComponent::Deserialize(const json& j) {
	isInitialized_ = true;
	if (j.contains("maxEnemies")) maxEnemies_ = j["maxEnemies"];
	if (j.contains("spawnInterval")) spawnInterval_ = j["spawnInterval"];
	if (j.contains("spawnRadius")) spawnRadius_ = j["spawnRadius"];
	if (j.contains("isSpawningEnabled")) isSpawningEnabled_ = j["isSpawningEnabled"];
	if (j.contains("fishScale")) fishScale_ = j["fishScale"];
	if (j.contains("fishColliderRadius")) fishColliderRadius_ = j["fishColliderRadius"];
	if (j.contains("isCrateSpawningEnabled")) isCrateSpawningEnabled_ = j["isCrateSpawningEnabled"];
	if (j.contains("maxCrates")) maxCrates_ = j["maxCrates"];
	if (j.contains("crateSpawnInterval")) crateSpawnInterval_ = j["crateSpawnInterval"];
	if (j.contains("crateSpawnRadius")) crateSpawnRadius_ = j["crateSpawnRadius"];
}