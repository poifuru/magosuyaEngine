#include "PCH.h"
#include "ColliderComponent.h"
#include "GameObject.h"
#include "CollisionManager.h"
#include "EnergyItemComponent.h"
#include "BoarderComponent.h"
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

	// ボーダーがアイテムに触れたら回収
	bool isMyPlayer = myObj->GetComponent<BoarderComponent>() != nullptr;
	bool isOtherPlayer = otherObj->GetComponent<BoarderComponent>() != nullptr;
	if (isMyPlayer) {
		if (auto* item = otherObj->GetComponent<EnergyItemComponent>()) {
			item->OnCollect();
		}
	} else if (isOtherPlayer) {
		if (auto* item = myObj->GetComponent<EnergyItemComponent>()) {
			item->OnCollect();
		}
	}
}