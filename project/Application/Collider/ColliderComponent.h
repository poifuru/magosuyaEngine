#pragma once
#include "Component.h"
#include "Collision.h"

class ColliderComponent : public Component, public CollisionObject {
public:
	ColliderComponent();
	~ColliderComponent() override;

	void Initialize() override;
	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "ColliderComponent"; }

	// コライダーの半径を設定する
	void SetRadius(float r) { radius_ = r; }

	// 衝突した時のコールバック（CollisionObjectの仮想関数をオーバーライド）
	void OnHit(CollisionObject* other) override;

	const Vector3& GetPrevPosition() const { return prevPosition_; }
private:
	float radius_ = 1.0f;
	Vector3 prevPosition_{};	// 前フレームの座標
};