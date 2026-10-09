#pragma once
#include <vector>
#include "Collision.h"

class CollisionManager {
public:
	// シングルトンインスタンスの取得
	static CollisionManager* GetInstance() {
		static CollisionManager instance;
		return &instance;
	}

	// オブジェクトの登録
	void RegisterObject(CollisionObject* obj);

	// オブジェクトの登録解除
	void UnregisterObject(CollisionObject* obj);

	// ゲームループで呼び出す更新処理
	void UpdateAllCollisions();

private:
	CollisionManager() = default;
	~CollisionManager() = default;

	// コピー・移動禁止
	CollisionManager(const CollisionManager&) = delete;
	CollisionManager& operator=(const CollisionManager&) = delete;
	CollisionManager(CollisionManager&&) = delete;
	CollisionManager& operator=(CollisionManager&&) = delete;
	// 形状を見て判定を分岐する関数
	bool CheckActualCollision(CollisionObject* a, CollisionObject* b);

private:
	std::vector<CollisionObject*> objects_;
};