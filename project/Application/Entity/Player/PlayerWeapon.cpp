#include "PCH.h"
#include "PlayerWeapon.h"
#include "GameObject.h"
#include "InputManager.h"
#include "RawInput.h"
#include "GamePad.h"
#include "MathFunction.h"
#include "CameraOrganizer.h"
#include "BaseCamera.h"
#include "MeshRendererComponent.h"
#include "Model.h"
#include "Skeleton.h"
#include "BulletComponent.h"
#include "ReticleComponent.h"
#include "ColliderComponent.h"
#include "BlendModeManager.h"
#include "../../../../Engine/Editor/ParticleEditor/ParticleSpawner.h"
#include <cmath>

void PlayerWeapon::Initialize() {
	cooltime_ = 0.0f;
	preTriggerR_ = 0.0f;
	harpoonSpeed_ = 120.0f;
	harpoonHomingStrength_ = 0.02f;
	harpoonMaxDistance_ = 18.0f;
}

void PlayerWeapon::Update(GameObject* gameObject, GameObject* reticleObject) {
	// クールタイム更新
	if (cooltime_ > 0.0f) {
		cooltime_ -= Time::GetDeltaTime();
	}

	// 射撃処理
	Shoot(gameObject, reticleObject);

	// レティクルへのハープーン有効射程（ロックオン距離）の自動同期
	if (reticleObject) {
		if (auto* reticleComp = reticleObject->GetComponent<ReticleComponent>()) {
			reticleComp->SetLockOnMaxDistance(harpoonMaxDistance_);
		}
	}
}

void PlayerWeapon::Shoot(GameObject* gameObject, GameObject* reticleObject) {
	if (!gameObject) return;
	InputManager* input = InputManager::GetInstance();

	bool isShootTriggered = input->GetRawInput()->TriggerMouse(0);
	if (input->GetGamePad()->IsConection()) {
		float triggerR = input->GetGamePad()->GetTrigger(LR::Right);
		if (triggerR > 0.5f && preTriggerR_ <= 0.5f) {
			isShootTriggered = true;
		}
		preTriggerR_ = triggerR;
	} else {
		preTriggerR_ = 0.0f;
	}

	if (isShootTriggered && cooltime_ <= 0.0f) {
		auto* context = gameObject->GetContext();
		if (!context || !context->gameObjects) return;

		// 弾用の GameObject を生成
		auto bulletObj = std::make_unique<GameObject>(context, "PlayerBullet");

		// 弾本体のメッシュ
		auto* meshRenderer = bulletObj->AddComponent<MeshRendererComponent>();
		meshRenderer->SetModel("Resources/bullet/bullet.obj");
		meshRenderer->SetTexture("Resources/bullet/bullet.png");

		// 弾を大きく見やすく拡大表示 (適正サイズ 1.2倍)
		bulletObj->GetTransform().scale = { 1.2f, 1.2f, 1.2f };

		// 水面の下に潜り込んでも深度テストで遮蔽されず、水面越しに弾モデルが常にくっきり描画される設定！
		meshRenderer->SetDepthEnable(false);
		meshRenderer->SetBlendMode(MyEngine::Rendering::BlendModeType::Alpha);
		meshRenderer->SetEnableLighting(false); // 暗闇や水深でも常に明るく目立つように！

		// 弾専用のアウトライン（黒色外枠ライン）用オブジェクトを生成
		auto outlineObj = std::make_unique<GameObject>(context, "BulletOutline");
		auto* outlineRenderer = outlineObj->AddComponent<MeshRendererComponent>();
		outlineRenderer->SetModel("Resources/bullet/bullet.obj");
		outlineRenderer->SetTexture("Resources/bullet/bullet.png");
		outlineRenderer->SetDepthEnable(false);
		if (auto* model = outlineRenderer->GetModel()) {
			if (auto* mat = model->GetMaterial()) {
				mat->SetShadingModel(MyEngine::Rendering::ShadingModel::OutlineObject);
				mat->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f }); // 黒色の輪郭線
			}
		}
		// 弾本体(1.2)よりほんの少し大きく膨らませて(1.28)外枠を作る
		outlineObj->GetTransform().scale = { 1.28f, 1.28f, 1.28f };
		outlineObj->Initialize();
		outlineObj->SetSerializable(false);

		// 弾の挙動コンポーネントと当たり判定コンポーネントを追加
		auto* bulletComp = bulletObj->AddComponent<BulletComponent>();
		auto* colliderComp = bulletObj->AddComponent<ColliderComponent>();

		// コンポーネントたちを初期化
		bulletObj->Initialize();

		// 生成した弾にプレイヤーで設定しているハープーンのパラメータを適用
		bulletComp->SetSpeed(harpoonSpeed_);
		bulletComp->SetHomingStrength(harpoonHomingStrength_);
		bulletComp->SetMaxDistance(harpoonMaxDistance_);
		bulletComp->SetOutlineObject(outlineObj.get()); // 💡 アウトラインオブジェクトの移動・寿命を同期！

		// 当たり判定の大きさを設定する（モデル拡大に合わせて 1.2f に拡張）
		colliderComp->SetRadius(1.2f);

		// レティクルがロックしている敵がいれば、弾に追尾対象としてセットする
		if (reticleObject) {
			if (auto* reticleComp = reticleObject->GetComponent<ReticleComponent>()) {
				if (auto* target = reticleComp->GetLockOnTarget()) {
					bulletComp->SetTarget(target);
				}
			}
		}

		// 開始位置を設定（Canon ノードの最新ワールド位置）
		Vector3 bulletSpawnPos = {
			gameObject->GetTransform().translate.x,
			gameObject->GetTransform().translate.y + 0.25f,
			gameObject->GetTransform().translate.z
		};

		if (auto* meshRendererComp = gameObject->GetComponent<MeshRendererComponent>()) {
			if (auto* model = meshRendererComp->GetModel()) {
				if (auto* canonNode = model->FindNode("Canon")) {
					Matrix4x4 playerWorld = Math::MakeAffineMatrix(
						gameObject->GetTransform().scale,
						gameObject->GetTransform().rotate,
						gameObject->GetTransform().translate
					);
					Matrix4x4 canonWorld = canonNode->localMatrix * playerWorld;
					bulletSpawnPos = { canonWorld.m[3][0], canonWorld.m[3][1], canonWorld.m[3][2] };
				}
			}
		}

		bulletObj->GetTransform().translate = bulletSpawnPos;

		// 方向の計算
		CameraData& cameraData = CameraOrganizer::GetInstance()->GetCameraData();
		Vector3 camPos = { cameraData.world.m[3][0], cameraData.world.m[3][1], cameraData.world.m[3][2] };
		Vector3 camForward = { cameraData.world.m[2][0], cameraData.world.m[2][1], cameraData.world.m[2][2] };

		Vector3 targetPos = Math::Add(camPos, Math::Multiply(100.0f, camForward)); // カメラ正面100m先

		// もし敵をロックオンしているなら、その敵の座標を直接狙う！
		if (reticleObject) {
			if (auto* reticleComp = reticleObject->GetComponent<ReticleComponent>()) {
				if (auto* lockOnEnemy = reticleComp->GetLockOnTarget()) {
					targetPos = lockOnEnemy->GetTransform().translate;
				}
			}
		}

		Vector3 direction = Math::Subtract(targetPos, bulletObj->GetTransform().translate);
		if (Math::Length(direction) > 0.001f) {
			direction = Math::Normalize(direction);
		} else {
			direction = { 0.0f, 0.0f, 1.0f };
		}

		bulletComp->SetDirection(direction);

		// 弾が進行方向を向くように回転を設定する
		Vector3 bulletRot = { 0.0f, 0.0f, 0.0f };
		if (Math::Length(direction) > 0.001f) {
			// ヨー回転（左右）の計算
			bulletRot.y = std::atan2(direction.x, direction.z);

			// ピッチ回転（上下）の計算
			float xzLength = std::sqrt(direction.x * direction.x + direction.z * direction.z);
			bulletRot.x = std::atan2(-direction.y, xzLength);
		}
		bulletObj->GetTransform().rotate = bulletRot; // 回転を適用

		// アウトライン用のトランスフォームも一致させて追加
		outlineObj->GetTransform().translate = bulletSpawnPos;
		outlineObj->GetTransform().rotate = bulletRot;

		// 弾発射時の位置から水飛沫マズルブラストを10個飛び散らせる！
		ParticleSpawner::SpawnWaterSplash(context, 
										{ bulletObj->GetTransform().translate.x, 
										bulletObj->GetTransform().translate.y,
										bulletObj->GetTransform().translate.z
										},
										10);

		// セーブ対象外
		bulletObj->SetSerializable(false);

		// シーンのオブジェクトリストに追加
		context->gameObjects->push_back(std::move(bulletObj));
		context->gameObjects->push_back(std::move(outlineObj));
		cooltime_ = 0.25f;
	}
}

void PlayerWeapon::ImGui() {
#ifdef USEIMGUI
	ImGui::Text("--- Harpoon Gun ---");
	ImGui::DragFloat("Harpoon Speed", &harpoonSpeed_, 1.0f, 10.0f, 300.0f);
	ImGui::DragFloat("Harpoon Max Distance (Range)", &harpoonMaxDistance_, 0.5f, 5.0f, 100.0f);
	ImGui::SliderFloat("Homing Strength", &harpoonHomingStrength_, 0.0f, 0.5f);
#endif
}

void PlayerWeapon::Serialize(json& j) const {
	j["harpoonSpeed"] = harpoonSpeed_;
	j["harpoonHomingStrength"] = harpoonHomingStrength_;
	j["harpoonMaxDistance"] = harpoonMaxDistance_;
}

void PlayerWeapon::Deserialize(const json& j) {
	if (j.contains("harpoonMaxDistance")) harpoonMaxDistance_ = j["harpoonMaxDistance"];
	if (j.contains("harpoonSpeed")) harpoonSpeed_ = j["harpoonSpeed"];
	if (j.contains("homingStrength")) {
		harpoonHomingStrength_ = j["homingStrength"];
	} else if (j.contains("harpoonHomingStrength")) {
		harpoonHomingStrength_ = j["harpoonHomingStrength"];
	}
}
