#include "PCH.h"
#include "CollisionManager.h"
#include "ColliderComponent.h"
#include "GameObject.h"
#include "MathFunction.h"

// 線分ABと点Cの最短距離の二乗を計算する数学関数
float SqDistPointSegment(const Vector3& A, const Vector3& B, const Vector3& C) {
	Vector3 AB = Math::Subtract(B, A);
	Vector3 AC = Math::Subtract(C, A);
	Vector3 BC = Math::Subtract(C, B);

	float e = Math::Dot(AC, AB);
	if (e <= 0.0f) return Math::Dot(AC, AC); // Aの外側

	float f = Math::Dot(AB, AB);
	if (e >= f) return Math::Dot(BC, BC); // Bの外側

	return Math::Dot(AC, AC) - (e * e) / f; // 線分の間
}

void CollisionManager::RegisterObject(CollisionObject* obj) {
	// すでに登録されている場合は、二重登録しないように弾く
	for (auto* o : objects_) {
		if (o == obj) return;
	}
	objects_.push_back(obj);
}

void CollisionManager::UnregisterObject(CollisionObject* obj) {
	for(auto it = objects_.begin(); it != objects_.end(); ++it) {
		if(*it == obj) {
			objects_.erase(it);
			break;
		}
	}
}

void CollisionManager::UpdateAllCollisions() {
	if(objects_.size() < 2) return;

	// 衝突したペアを一時的に記録するリストを用意する
	std::vector<std::pair<CollisionObject*, CollisionObject*>> collidedPairs;

	// 重複のない総当たり判定（i < j）にすることで自分自身や重複判定を防ぐ
	for(size_t i = 0; i < objects_.size() - 1; ++i) {
		for(size_t j = i + 1; j < objects_.size(); ++j) {
			CollisionObject* a = objects_[i];
			CollisionObject* b = objects_[j];
			if(CheckActualCollision(a, b)) {
				// 当たっていたら互いにリストに追加する
				collidedPairs.push_back({a, b});
			}
		}
	}

	// ループが終わった後に、安全に OnCollision を通知する
	for(const auto& pair : collidedPairs) {
		// 相手がすでに破棄されている（objects_ から消えている）可能性を考慮して
		// まだ objects_ に存在している場合のみ OnCollision を呼ぶ
		bool aExists = std::find(objects_.begin(), objects_.end(), pair.first) != objects_.end();
		bool bExists = std::find(objects_.begin(), objects_.end(), pair.second) != objects_.end();

		if(aExists && bExists) {
			pair.first->OnHit(pair.second);
			pair.second->OnHit(pair.first);
		}
	}
}

bool CollisionManager::CheckActualCollision(CollisionObject* a, CollisionObject* b) {
	// ColliderComponentへのキャストを行う
	auto* colA = dynamic_cast<ColliderComponent*>(a);
	auto* colB = dynamic_cast<ColliderComponent*>(b);

	// 関数内での OnCollision 呼び出しをやめて、純粋に判定結果（bool）だけを返す
	if (a->GetCollisionType() == CollisionType::AABB && 
			 b->GetCollisionType() == CollisionType::AABB)
	{
		return Math::IsCollision(a->GetAABB(), b->GetAABB());
	}
	else if (a->GetCollisionType() == CollisionType::AABB && 
			 b->GetCollisionType() == CollisionType::Sphere)
	{
		return Math::IsCollision(a->GetAABB(), b->GetSphere());
	}
	else if (a->GetCollisionType() == CollisionType::Sphere && 
			 b->GetCollisionType() == CollisionType::AABB)
	{
		return Math::IsCollision(b->GetAABB(), a->GetSphere());
	}
	return false;
}
