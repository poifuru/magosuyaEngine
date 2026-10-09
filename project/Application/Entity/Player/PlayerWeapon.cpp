#include "PCH.h"
#include "PlayerWeapon.h"
#include "PlayerHealth.h"
#include "GameObject.h"
#include "InputManager.h"
#include "RawInput.h"
#include "GamePad.h"
#include "MathFunction.h"
#include "CameraOrganizer.h"
#include "BaseCamera.h"
#include "MeshRendererComponent.h"
#include "SpriteComponent.h"
#include "NumberDrawerComponent.h"
#include "Model.h"
#include "Skeleton.h"
#include "BulletComponent.h"
#include "ReticleComponent.h"
#include "ColliderComponent.h"
#include "BlendModeManager.h"
#include "../../../../Engine/Editor/ParticleEditor/ParticleSpawner.h"
#include <cmath>

PlayerWeapon::~PlayerWeapon() {
	if (ammoBorderObj_) ammoBorderObj_->Destroy();
	if (ammoBgObj_) ammoBgObj_->Destroy();
	if (ammoBarObj_) ammoBarObj_->Destroy();
	if (ammoNumObj_) ammoNumObj_->Destroy();
}

void PlayerWeapon::Initialize() {
	cooltime_ = 0.0f;
	preTriggerR_ = 0.0f;
	fireInterval_ = 0.15f;
	harpoonSpeed_ = 120.0f;
	harpoonHomingStrength_ = 0.02f;
	harpoonMaxDistance_ = 80.0f;

	ammo_ = 15;
	maxAmmo_ = 15;
	reloadCost_ = 10;
	reloadDuration_ = 0.8f;
	reloadTimer_ = 0.0f;
	isReloading_ = false;

	ammoBorderObj_ = nullptr;
	ammoBgObj_ = nullptr;
	ammoBarObj_ = nullptr;
	ammoNumObj_ = nullptr;
	uiCreated_ = false;
}

void PlayerWeapon::Reload(PlayerHealth* playerHealth) {
	if (isReloading_ || ammo_ >= maxAmmo_) return;
	if (!playerHealth) return;

	// 電力を消費してリロード開始
	if (playerHealth->ConsumeHealth(reloadCost_)) {
		isReloading_ = true;
		reloadTimer_ = reloadDuration_;
	}
}

void PlayerWeapon::Update(GameObject* gameObject, GameObject* reticleObject, PlayerHealth* playerHealth) {
	if (!gameObject) return;

	// UI生成（未生成または破棄されていた場合に安全に再生成）
	if (!uiCreated_ || !ammoBarObj_ || ammoBarObj_->IsDead()) {
		uiCreated_ = false;
		CreateUI(gameObject);
	}

	// クールタイム更新
	if (cooltime_ > 0.0f) {
		cooltime_ -= Time::GetDeltaTime();
	}

	// リロードタイマー更新
	if (isReloading_) {
		reloadTimer_ -= Time::GetDeltaTime();
		if (reloadTimer_ <= 0.0f) {
			reloadTimer_ = 0.0f;
			isReloading_ = false;
			ammo_ = maxAmmo_;
		}
	}

	// 手動リロード（RキーまたはゲームパッドのXボタン）
	InputManager* input = InputManager::GetInstance();
	if (input) {
		bool reloadTriggered = false;
		if (input->GetRawInput() && input->GetRawInput()->Trigger('R')) {
			reloadTriggered = true;
		}
		if (input->GetGamePad() && input->GetGamePad()->IsConection()) {
			if (input->GetGamePad()->TriggerButton(Button::X)) {
				reloadTriggered = true;
			}
		}
		if (reloadTriggered) {
			Reload(playerHealth);
		}
	}

	// 射撃処理
	Shoot(gameObject, reticleObject, playerHealth);

	// レティクルへのハープーン有効射程（ロックオン距離）の自動同期
	if (reticleObject) {
		if (auto* reticleComp = reticleObject->GetComponent<ReticleComponent>()) {
			reticleComp->SetLockOnMaxDistance(harpoonMaxDistance_);
		}
	}

	// 残弾UI更新
	UpdateUI();
}

void PlayerWeapon::Shoot(GameObject* gameObject, GameObject* reticleObject, PlayerHealth* playerHealth) {
	if (!gameObject) return;

	// リロード中は発射不可
	if (isReloading_) return;

	InputManager* input = InputManager::GetInstance();

	// マウス左クリック長押しで連射
	bool isShootTriggered = input->GetRawInput()->PushMouse(0);
	if (input->GetGamePad()->IsConection()) {
		float triggerR = input->GetGamePad()->GetTrigger(LR::Right);
		if (triggerR > 0.5f) {
			isShootTriggered = true;
		}
		preTriggerR_ = triggerR;
	} else {
		preTriggerR_ = 0.0f;
	}

	if (isShootTriggered && cooltime_ <= 0.0f) {
		// 弾切れなら自動リロードを試みる
		if (ammo_ <= 0) {
			Reload(playerHealth);
			cooltime_ = 0.2f;
			return;
		}

		auto* context = gameObject->GetContext();
		if (!context || !context->gameObjects) return;

		// 弾を1発消費
		ammo_--;

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

		// 開始位置を設定（左右砲身ノード Canon_L / Canon_R の最新ワールド位置から交互に発射）
		Vector3 bulletSpawnPos = {
			gameObject->GetTransform().translate.x,
			gameObject->GetTransform().translate.y + 0.25f,
			gameObject->GetTransform().translate.z
		};

		if (auto* meshRendererComp = gameObject->GetComponent<MeshRendererComponent>()) {
			if (auto* model = meshRendererComp->GetModel()) {
				// 左右どちらの砲身を使うか選択
				const char* targetCanonName = (shootSide_ == 0) ? "Canon_L" : "Canon_R";
				auto* canonNode = model->FindNode(targetCanonName);
				if (!canonNode) {
					canonNode = (shootSide_ == 0) ? model->FindNode("Canon") : model->FindNode("Canon.001");
				}
				if (!canonNode) {
					canonNode = model->FindNode("Canon");
				}

				Matrix4x4 playerWorld = Math::MakeAffineMatrix(
					gameObject->GetTransform().scale,
					gameObject->GetTransform().rotate,
					gameObject->GetTransform().translate
				);

				if (canonNode) {
					Matrix4x4 canonWorld = canonNode->localMatrix * playerWorld;
					bulletSpawnPos = { canonWorld.m[3][0], canonWorld.m[3][1], canonWorld.m[3][2] };
				} else {
					// ノードが見当たらない場合のフォールバック（左右オフセット ±1.05m、前方 +1.8m、高さ -0.2m）
					float sideOffset = (shootSide_ == 0) ? -1.05f : 1.05f;
					Vector3 localOffset = { sideOffset, -0.2f, 1.8f };
					bulletSpawnPos = Math::Transform(localOffset, playerWorld);
				}
			}
		}

		// 次回発射用に左右を交互に切り替え（0: 左 ↔ 1: 右）
		shootSide_ = 1 - shootSide_;

		bulletObj->GetTransform().translate = bulletSpawnPos;

		// 方向の計算
		const CameraData& cameraData = CameraOrganizer::GetInstance()->GetCameraData();
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
		cooltime_ = fireInterval_;
	}
}

void PlayerWeapon::CreateUI(GameObject* gameObject) {
	if (!gameObject) return;
	auto* context = gameObject->GetContext();
	if (!context || !context->gameObjects) return;

	// 外枠（ダークグレー）
	auto borderObj = std::make_unique<GameObject>(context, "PlayerAmmo_Border");
	auto* borderSprite = borderObj->AddComponent<SpriteComponent>();
	borderSprite->SetTexture("Resources/human/white.png");
	borderSprite->SetAnchorPoint({ 0.0f, 0.0f });
	borderSprite->SetPosition({ 304.0f, 638.0f });
	borderSprite->SetSize({ 134.0f, 26.0f });
	borderSprite->SetColor({ 0.05f, 0.05f, 0.08f, 0.9f });
	borderSprite->SetLayer(3);
	borderObj->Initialize();
	borderObj->SetSerializable(false);
	ammoBorderObj_ = borderObj.get();
	context->gameObjects->push_back(std::move(borderObj));

	// ゲージ背景（暗いシアン系）
	auto bgObj = std::make_unique<GameObject>(context, "PlayerAmmo_BG");
	auto* bgSprite = bgObj->AddComponent<SpriteComponent>();
	bgSprite->SetTexture("Resources/human/white.png");
	bgSprite->SetAnchorPoint({ 0.0f, 0.0f });
	bgSprite->SetPosition({ 306.0f, 640.0f });
	bgSprite->SetSize({ 130.0f, 22.0f });
	bgSprite->SetColor({ 0.08f, 0.15f, 0.2f, 0.85f });
	bgSprite->SetLayer(4);
	bgObj->Initialize();
	bgObj->SetSerializable(false);
	ammoBgObj_ = bgObj.get();
	context->gameObjects->push_back(std::move(bgObj));

	// 残弾ゲージ本体
	auto barObj = std::make_unique<GameObject>(context, "PlayerAmmo_Bar");
	auto* barSprite = barObj->AddComponent<SpriteComponent>();
	barSprite->SetTexture("Resources/human/white.png");
	barSprite->SetAnchorPoint({ 0.0f, 0.0f });
	barSprite->SetPosition({ 306.0f, 640.0f });
	barSprite->SetSize({ 130.0f, 22.0f });
	barSprite->SetColor({ 0.2f, 0.75f, 0.95f, 1.0f });
	barSprite->SetLayer(5);
	barObj->Initialize();
	barObj->SetSerializable(false);
	ammoBarObj_ = barObj.get();
	context->gameObjects->push_back(std::move(barObj));

	// 残弾数デジタル表示
	auto numObj = std::make_unique<GameObject>(context, "PlayerAmmo_Num");
	auto* numDrawer = numObj->AddComponent<NumberDrawerComponent>();
	numDrawer->SetAnchorPoint({ 0.0f, 0.0f });
	numDrawer->SetPosition({ 446.0f, 640.0f });
	numDrawer->SetSize({ 14.0f, 22.0f });
	numDrawer->SetSpacing(2.0f);
	numDrawer->SetAlignment(NumberDrawerComponent::Alignment::Left);
	numDrawer->SetValue(ammo_);
	numDrawer->SetColor({ 0.9f, 0.95f, 1.0f, 1.0f });
	numDrawer->SetLayer(5);
	numObj->Initialize();
	numObj->SetSerializable(false);
	ammoNumObj_ = numObj.get();
	context->gameObjects->push_back(std::move(numObj));

	uiCreated_ = true;
}

void PlayerWeapon::UpdateUI() {
	if (!ammoBarObj_) return;

	float ratio = 0.0f;
	if (maxAmmo_ > 0) {
		ratio = static_cast<float>(ammo_) / static_cast<float>(maxAmmo_);
		if (ratio < 0.0f) ratio = 0.0f;
		if (ratio > 1.0f) ratio = 1.0f;
	}

	if (auto* barSprite = ammoBarObj_->GetComponent<SpriteComponent>()) {
		if (isReloading_) {
			// リロード中は黄色点滅で装填進行状況を表示
			bool flash = (static_cast<int>(reloadTimer_ * 10.0f) % 2 == 0);
			float reloadProgress = 1.0f - (reloadTimer_ / reloadDuration_);
			barSprite->SetSize({ 130.0f * reloadProgress, 22.0f });
			barSprite->SetColor(flash ? Vector4{ 1.0f, 0.9f, 0.2f, 1.0f } : Vector4{ 0.7f, 0.6f, 0.1f, 0.8f });
		} else {
			barSprite->SetSize({ 130.0f * ratio, 22.0f });

			Vector4 barColor;
			if (ratio > 0.35f) {
				barColor = { 0.2f, 0.75f, 0.95f, 1.0f }; // シアン
			} else if (ratio > 0.0f) {
				barColor = { 0.95f, 0.6f, 0.15f, 1.0f };  // オレンジ（残弾少）
			} else {
				barColor = { 0.8f, 0.2f, 0.2f, 0.7f };   // 赤（空）
			}
			barSprite->SetColor(barColor);
		}
	}

	if (ammoNumObj_) {
		if (auto* numDrawer = ammoNumObj_->GetComponent<NumberDrawerComponent>()) {
			numDrawer->SetValue(ammo_);
			if (isReloading_) {
				numDrawer->SetColor({ 1.0f, 0.9f, 0.2f, 1.0f });
			} else if (ammo_ <= 3) {
				numDrawer->SetColor({ 0.95f, 0.6f, 0.15f, 1.0f });
			} else {
				numDrawer->SetColor({ 0.9f, 0.95f, 1.0f, 1.0f });
			}
		}
	}
}

void PlayerWeapon::ImGui() {
#ifdef USEIMGUI
	ImGui::Text("--- Harpoon Gun ---");
	ImGui::DragFloat("Harpoon Speed", &harpoonSpeed_, 1.0f, 10.0f, 300.0f);
	ImGui::DragFloat("Harpoon Max Distance (Range)", &harpoonMaxDistance_, 0.5f, 5.0f, 300.0f);
	ImGui::DragFloat("Fire Interval", &fireInterval_, 0.01f, 0.05f, 1.0f);
	ImGui::SliderFloat("Homing Strength", &harpoonHomingStrength_, 0.0f, 0.5f);

	ImGui::Separator();
	ImGui::Text("--- Magazine & Reload ---");
	ImGui::DragInt("Ammo", &ammo_, 1, 0, maxAmmo_);
	ImGui::DragInt("Max Ammo", &maxAmmo_, 1, 1, 100);
	ImGui::DragInt("Reload Cost (Power)", &reloadCost_, 1, 0, 100);
	ImGui::DragFloat("Reload Duration", &reloadDuration_, 0.05f, 0.1f, 5.0f);
	ImGui::Text("Is Reloading: %s (Timer: %.2f)", isReloading_ ? "YES" : "NO", reloadTimer_);
#endif
}

void PlayerWeapon::Serialize(json& j) const {
	j["harpoonSpeed"] = harpoonSpeed_;
	j["harpoonHomingStrength"] = harpoonHomingStrength_;
	j["harpoonMaxDistance"] = harpoonMaxDistance_;
	j["fireInterval"] = fireInterval_;
	j["maxAmmo"] = maxAmmo_;
	j["reloadCost"] = reloadCost_;
	j["reloadDuration"] = reloadDuration_;
}

void PlayerWeapon::Deserialize(const json& j) {
	if (j.contains("harpoonMaxDistance")) harpoonMaxDistance_ = j["harpoonMaxDistance"];
	if (j.contains("harpoonSpeed")) harpoonSpeed_ = j["harpoonSpeed"];
	if (j.contains("fireInterval")) fireInterval_ = j["fireInterval"];
	if (j.contains("maxAmmo")) {
		maxAmmo_ = j["maxAmmo"];
		ammo_ = maxAmmo_;
	}
	if (j.contains("reloadCost")) reloadCost_ = j["reloadCost"];
	if (j.contains("reloadDuration")) reloadDuration_ = j["reloadDuration"];
	if (j.contains("homingStrength")) {
		harpoonHomingStrength_ = j["homingStrength"];
	} else if (j.contains("harpoonHomingStrength")) {
		harpoonHomingStrength_ = j["harpoonHomingStrength"];
	}
}
