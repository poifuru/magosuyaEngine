#pragma once
#include "Component.h"

class EnemyManagerComponent : public Component {
public:
	EnemyManagerComponent() = default;
	~EnemyManagerComponent() override = default;

	void Initialize() override;
	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "EnemyManagerComponent"; }

	void SetSpawningEnabled(bool enable) { isSpawningEnabled_ = enable; }
	bool IsSpawningEnabled() const { return isSpawningEnabled_; }
	void ClearAllEnemies();

	float GetFishScale() const { return fishScale_; }
	void SetFishScale(float s) { fishScale_ = s; }
	float GetFishColliderRadius() const { return fishColliderRadius_; }
	void SetFishColliderRadius(float r) { fishColliderRadius_ = r; }

private:
	// 敵をスポーンさせる処理
	void SpawnEnemy();

private:
	bool isSpawningEnabled_ = true; // スポーン有効フラグ
	int maxEnemies_ = 5;         // 同時に存在できる敵の最大数

	float spawnInterval_ = 3.0f; // スポーン間隔（秒）
	float spawnTimer_ = 0.0f;    // 残り時間タイマー
	float spawnRadius_ = 35.0f;  // プレイヤーからのスポーン距離（遠目に変更）

	float fishScale_ = 2.5f;             // 魚エネミーのスケール倍率（デフォルト2.5倍）
	float fishColliderRadius_ = 2.0f;    // 魚エネミーのコライダー半径（デフォルト2.0f）
};