#pragma once
#include <variant>
#include "Geometry.h"

// コライダーの形状
enum class CollisionType {
	Sphere,
	AABB,
};

// 衝突イベントを受け取るための基底クラス
class CollisionObject {
public:
	// 球として初期化するコンストラクタ
	CollisionObject(const Sphere& sphere) 
		: type_(CollisionType::Sphere), geometry_(sphere) {}

	// AABBとして初期化するコンストラクタ
	CollisionObject(const AABB& aabb) 
		: type_(CollisionType::AABB), geometry_(aabb) {}

	virtual ~CollisionObject() = default;

	// 衝突したときにマネージャーから呼ばれる仮想関数
	virtual void OnHit(CollisionObject* other) = 0;

	CollisionType GetCollisionType() const { return type_; }

	const Sphere& GetSphere() const { return std::get<Sphere>(geometry_); }
	const AABB& GetAABB() const { return std::get<AABB>(geometry_); }

private:
	CollisionType type_;

	// AABBか球のどちらか一方を格納する
	std::variant<AABB, Sphere> geometry_;
};