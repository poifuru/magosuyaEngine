#include "PCH.h"
#include "EnemyManagerComponent.h"
#include "GameObject.h"
#include "BirdEnemyComponent.h"
#include "FishEnemyComponent.h"
#include "ColliderComponent.h"
#include "BaseScene.h" // SceneContext や gameObjects へのアクセス用
#include "PlayerComponent.h" // プレイヤー検索用
#include "MeshRendererComponent.h"

void EnemyManagerComponent::Initialize() {
	if (isInitialized_) return;

	isInitialized_ = true;
	// タイマーの初期化
	spawnTimer_ = spawnInterval_;
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

	if (!isSpawningEnabled_) return;

	// 時間経過でタイマーを減算
	spawnTimer_ -= Time::GetDeltaTime();
	if (spawnTimer_ <= 0.0f) {
		spawnTimer_ = spawnInterval_;

		SpawnEnemy();
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

void EnemyManagerComponent::ImGui() {
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
	if (gameObject_ && gameObject_->GetContext() && gameObject_->GetContext()->activeGameObjects) {
		for (const auto& obj : *(gameObject_->GetContext()->activeGameObjects)) {
			if (obj->GetComponent<BirdEnemyComponent>() != nullptr || obj->GetComponent<FishEnemyComponent>() != nullptr) {
				currentEnemyCount++;
			}
		}
	}
	ImGui::Text("Current Enemies: %d / %d", currentEnemyCount, maxEnemies_);
}

void EnemyManagerComponent::Serialize(json& j) const {
	j["type"] = "EnemyManagerComponent";
	j["maxEnemies"] = maxEnemies_;
	j["spawnInterval"] = spawnInterval_;
	j["spawnRadius"] = spawnRadius_;
	j["isSpawningEnabled"] = isSpawningEnabled_;
	j["fishScale"] = fishScale_;
	j["fishColliderRadius"] = fishColliderRadius_;
}

void EnemyManagerComponent::Deserialize(const json& j) {
	isInitialized_ = true;
	if (j.contains("maxEnemies")) maxEnemies_ = j["maxEnemies"];
	if (j.contains("spawnInterval")) spawnInterval_ = j["spawnInterval"];
	if (j.contains("spawnRadius")) spawnRadius_ = j["spawnRadius"];
	if (j.contains("isSpawningEnabled")) isSpawningEnabled_ = j["isSpawningEnabled"];
	if (j.contains("fishScale")) fishScale_ = j["fishScale"];
	if (j.contains("fishColliderRadius")) fishColliderRadius_ = j["fishColliderRadius"];
}