#include "PCH.h"
#include "ColliderComponent.h"
#include "GameObject.h"
#include "CollisionManager.h"
#include "BirdEnemyComponent.h"
#include "FishEnemyComponent.h"
#include "Boss.h"
#include "FloatingCrateComponent.h"
#include "EnergyItemComponent.h"
#include "PlayerComponent.h"
#include "MathFunction.h"

// デフォルトは半径1の球として CollisionObject を初期化
ColliderComponent::ColliderComponent() 
	: CollisionObject(Sphere{{0.0f, 0.0f, 0.0f}, 1.0f}) {}

ColliderComponent::~ColliderComponent() {
	CollisionManager::GetInstance()->UnregisterObject(this);
}

void ColliderComponent::Initialize() {
	// 初期化時にも、親オブジェクトの位置をコライダーの球の中心に同期させる！
	if (gameObject_) {
		Sphere& sphere = const_cast<Sphere&>(GetSphere());
		sphere.center = gameObject_->GetTransform().translate;
		sphere.radius = radius_;

		prevPosition_ = sphere.center; 
	}

	// 登録処理はここで1回だけ呼ぶようにする！
	CollisionManager::GetInstance()->RegisterObject(this);

	if (isInitialized_) return;
	isInitialized_ = true;

	radius_ = 1.0f;

	// コライダーマネージャーに自身を登録！
	CollisionManager::GetInstance()->RegisterObject(this);
}

void ColliderComponent::Update() {
	if (!gameObject_) return;

	// 位置が更新される前に「前フレームの座標」として保存する！
	prevPosition_ = GetSphere().center;

	// 親GameObjectの座標に合わせて、コライダーの球の中心座標を更新する
	Vector3 myPos = gameObject_->GetTransform().translate;

	// CollisionObject内部の幾何データを更新
	Sphere& sphere = const_cast<Sphere&>(GetSphere());
	sphere.center = myPos;
	sphere.radius = radius_;
}

void ColliderComponent::ImGui() {
	// エディタ上でコライダーの半径をドラッグ調整できるようにする
	ImGui::DragFloat("Radius", &radius_, 0.1f, 0.01f, 50.0f);
}

void ColliderComponent::Serialize(json& j) const {
	j["type"] = "ColliderComponent";
	j["radius"] = radius_;
}

void ColliderComponent::Deserialize(const json& j) {
	isInitialized_ = true; // ロードしたので初期化済みフラグを立てる！
	if (j.contains("radius")) radius_ = j["radius"];
}

void ColliderComponent::OnCollision(CollisionObject* other) {
	// 衝突相手のコライダーコンポーネントを取得
	auto* otherCollider = dynamic_cast<ColliderComponent*>(other);
	if (!otherCollider) return;

	GameObject* otherObj = otherCollider->GetGameObject();
	GameObject* myObj = GetGameObject();
	if (!otherObj || !myObj) return;

	// ★【デバッグ用追加】何と何が衝突したかを Visual Studio の出力ウィンドウに表示する！
	char debugMsg[256];
	sprintf_s(debugMsg, "[Collision] %s <-> %s (Dist: %.2f)\n", 
			  myObj->GetName().c_str(), 
			  otherObj->GetName().c_str(),
			  Math::Length(Math::Subtract(myObj->GetTransform().translate, otherObj->GetTransform().translate))); // 距離も測る
	OutputDebugStringA(debugMsg);

	bool isMyEnemy = (myObj->GetComponent<BirdEnemyComponent>() != nullptr || 
					  myObj->GetComponent<FishEnemyComponent>() != nullptr ||
					  myObj->GetComponent<BossComponent>() != nullptr ||
					  myObj->GetName() == "Enemy" ||
					  myObj->GetName() == "Boss");
	bool isOtherEnemy = (otherObj->GetComponent<BirdEnemyComponent>() != nullptr || 
						 otherObj->GetComponent<FishEnemyComponent>() != nullptr ||
						 otherObj->GetComponent<BossComponent>() != nullptr ||
						 otherObj->GetName() == "Enemy" ||
						 otherObj->GetName() == "Boss");

	bool isMyPlayer = (myObj->GetName() == "Player" || myObj->GetComponent<PlayerComponent>() != nullptr);
	bool isOtherPlayer = (otherObj->GetName() == "Player" || otherObj->GetComponent<PlayerComponent>() != nullptr);

	// 1. 自分が「敵」または「木箱」で、相手が「弾」なら被弾処理
	if (otherObj->GetName() == "PlayerBullet") {
		if (auto* crate = myObj->GetComponent<FloatingCrateComponent>()) {
			crate->TakeDamage(1);
			CollisionManager::GetInstance()->UnregisterObject(this);
			otherObj->Destroy();
			return;
		}
	}

	if (isMyEnemy && otherObj->GetName() == "PlayerBullet") {
		bool alreadyDead = false;
		if (auto* boss = myObj->GetComponent<BossComponent>()) {
			alreadyDead = boss->IsDead();
			boss->TakeDamage(1);
			if (boss->IsDead()) {
				CollisionManager::GetInstance()->UnregisterObject(this);
			}
		}
		else if (auto* bird = myObj->GetComponent<BirdEnemyComponent>()) {
			alreadyDead = bird->IsDead();
			bird->TakeDamage(1);
			if (bird->IsDead()) {
				CollisionManager::GetInstance()->UnregisterObject(this);
			}
		}
		else if (auto* fish = myObj->GetComponent<FishEnemyComponent>()) {
			alreadyDead = fish->IsDead();
			fish->TakeDamage(1);
			if (fish->IsDead()) {
				CollisionManager::GetInstance()->UnregisterObject(this);
			}
		}
		else {
			myObj->Destroy();
			CollisionManager::GetInstance()->UnregisterObject(this);
		}

		otherObj->Destroy();
	}

	// 2. 自分が「プレイヤー」で、相手が「電力アイテム」ならアイテム回収
	if (isMyPlayer) {
		if (auto* item = otherObj->GetComponent<EnergyItemComponent>()) {
			item->OnCollect(myObj->GetComponent<PlayerComponent>());
			return;
		}
	}
	else if (isOtherPlayer) {
		if (auto* item = myObj->GetComponent<EnergyItemComponent>()) {
			item->OnCollect(otherObj->GetComponent<PlayerComponent>());
			return;
		}
	}

	// 3. 自分が「プレイヤー」で、相手が「敵」の場合の被弾ダメージ処理
	if (isMyPlayer && isOtherEnemy) {
		if (auto* playerComp = myObj->GetComponent<PlayerComponent>()) {
			int damage = 15; // デフォルト（魚など）
			if (otherObj->GetComponent<BossComponent>() || otherObj->GetName() == "Boss") {
				damage = 30; // ボスからの強撃
			} else if (otherObj->GetComponent<BirdEnemyComponent>()) {
				damage = 20; // 鳥の急降下攻撃
			}
			playerComp->TakeDamage(damage);
		}
	}
}