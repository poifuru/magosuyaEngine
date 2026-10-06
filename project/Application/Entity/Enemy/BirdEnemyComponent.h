#pragma once
#include "Component.h"

enum class BirdState {
	Patrol, // 遠方警戒・待機
	Circle, // プレイヤー周りを威嚇旋回
	Dive,   // 急降下
	Ascent  // 通り抜け上昇
};

class BirdEnemyComponent : public Component {
public:
	BirdEnemyComponent() = default;
	~BirdEnemyComponent() override = default;

	void Initialize() override;
	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "BirdEnemyComponent"; }

	BirdState GetState() const { return state_; }
	float GetAngle() const { return angle_; }
	int GetFormationIndex() const { return formationIndex_; }

	// フォーメーション（隊列）情報の更新
	void SetFormationInfo(int index, int total) {
		formationIndex_ = index;
		totalBirds_ = (total > 0) ? total : 1;
	}

public:
	// 被弾処理・ステータス
	void TakeDamage(int damage);
	void OnDead();
	bool IsDead() const { return isDead_; }

	int GetHp() const { return hp_; }
	int GetMaxHp() const { return maxHp_; }
	void SetHp(int hp) { hp_ = hp; }
	void SetMaxHp(int maxHp) { maxHp_ = maxHp; }

private:
	int hp_ = 2;                  // 体力（鳥は2発で撃破）
	int maxHp_ = 2;               // 最大体力
	float invincibilityTimer_ = 0.0f;     // 被弾後の無敵・点滅タイマー
	float invincibilityDuration_ = 0.15f; // 無敵時間

	Vector3 center_{};     // 円運動の中心
	Vector3 patrolCenter_{}; // スポーン時の自立待機中心
	float radius_ = 18.0f;  // 半径（遠目に変更）
	float currentRadius_ = 18.0f; // 切り替え時の実効半径
	float speed_ = 0.7f;    // 回転スピード（ラジアン/秒）
	float angle_ = 0.0f;    // 現在の角度（ラジアン）
	bool isInitialized_ = false; // リセット防止

	int formationIndex_ = 0; // 隊列内のインデックス
	int totalBirds_ = 1;     // 隊列の総数

	// 挙動用の追加メンバ
	BirdState state_ = BirdState::Patrol;
	float stateTimer_ = 0.0f;
	Vector3 diveTarget_{};
	Vector3 diveStartPos_{};
	Vector3 diveHorizDir_{ 1.0f, 0.0f, 0.0f }; // ダイブ時の水平移動方向

	float circleDuration_ = 7.5f; // 旋回時間を長めにしてじっくりタメを作る
	float detectionRange_ = 25.0f; // プレイヤーを認識して旋回に入る縄張り範囲
	float diveSpeed_ = 25.0f;      // 突撃速度
	float ascentSpeed_ = 12.0f;    // 上昇速度
	float baseHeight_ = 4.0f;     // 基本の高度
	float rotLerpSpeed_ = 5.0f;   // 回転補間の速度

	bool isDead_ = false;       // 死亡フラグ
	float deathTimer_ = 0.0f;   // 死亡演出タイマー
	Vector3 originalScale_{};   // 死亡時の初期スケール
	Vector3 originalPosition_{}; // 死亡時の初期座標
};