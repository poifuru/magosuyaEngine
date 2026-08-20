#pragma once
#include "Component.h"

class BulletComponent : public Component {
public:
	BulletComponent() = default;
	~BulletComponent() override;

	void Initialize() override;
	void Update() override;

	const char* GetName() const override { return "BulletComponent"; }
	void SetDirection(const Vector3& dir) { direction_ = dir; }
	void SetSpeed(float speed) { speed_ = speed; }
	void SetHomingStrength(float strength) { homingStrength_ = strength; }
	void SetMaxDistance(float dist) { maxDistance_ = dist; }
	float GetMaxDistance() const { return maxDistance_; }

	void SetTarget(GameObject* target) { target_ = target; }
	void SetOutlineObject(GameObject* outlineObj) { outlineObj_ = outlineObj; }

private:
	Vector3 direction_{};			// 向き
	Vector3 startPosition_{};       // 発射初期座標
	float speed_ = 80.0f;			// スピード
	float activeTimer_ = 5.0f;		// 有効時間
	float maxDistance_ = 18.0f;     // 有効射程距離（メートル）
	float homingStrength_ = 0.1f;	// 追尾力(%)
	bool hasRecordedStart_ = false; // 初期座標記録フラグ
	bool isSubmerged_ = false;       // 水面下潜入フラグ

	GameObject* target_ = nullptr; // 追尾対象
	GameObject* outlineObj_ = nullptr; // アウトライン用オブジェクト
};