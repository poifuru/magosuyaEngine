#include "PCH.h"
#include "PlayScene.h"
#include "Renderer.h"
#include "RenderSystem.h"
#include "CameraOrganizer.h"
#include "LevelEditor.h"
#include "ComponentType.h"
#include "InputManager.h"
#include "RawInput.h"
#include "CollisionManager.h"
#include "GraphicsDevice.h"
#include "LightManager.h"
#include "PostEffectManager.h"
#include "Dissolve.h"
#include "TextureManager.h"

PlayScene::PlayScene() = default;
PlayScene::~PlayScene() = default;

void PlayScene::Initialize() {
	if (!context_) return;

#ifdef USEIMGUI
	levelEditor_ = std::make_unique<LevelEditor>();
	levelEditor_->Initialize(context_);

	// 初回ロード時に defaultScene.json が存在すれば読み込む
	const std::string defaultScenePath = "Resources/Scene/defaultScene.json";
	if (std::filesystem::exists(defaultScenePath)) {
		levelEditor_->LoadScene(defaultScenePath, gameObjects_, selectedObject_);
	}
#else
	// リリースビルドでも、シーンのロードだけは行う
	const std::string defaultScenePath = "Resources/Scene/defaultScene.json";
	if (std::filesystem::exists(defaultScenePath)) {
		LevelEditor tempEditor;
		tempEditor.Initialize(context_);
		tempEditor.LoadScene(defaultScenePath, gameObjects_, selectedObject_);
	}
#endif

	// --- カメラの初期優先度設定 ---
	for (auto& obj : gameObjects_) {
		// デバッグカメラ
		if (auto* debugCam = obj->GetComponent<VirtualDebugCamera>()) {
#ifdef USEIMGUI
			debugCam->SetPriority(20); // 開発用ビルドなら優先度高
#else
			debugCam->SetPriority(10); // リリースビルドなら優先度低
#endif
		}
		// 追従カメラ
		if (auto* followCam = obj->GetComponent<VirtualFollowCamera>()) {
#ifdef USEIMGUI
			followCam->SetPriority(10); // 開発用ビルドなら優先度低
#else
			followCam->SetPriority(20); // リリースビルドなら優先度高
#endif
		}
	}

	// コンテキストにリストのポインタをセットする
	context_->gameObjects = &createQueue_;

	// 本番の生存リストをセット
	context_->activeGameObjects = &gameObjects_;

	// ライトマネージャーの初期化
	lightManager_ = std::make_unique<LightManager>();
	lightManager_->Initialize(context_->graphicsDevice->GetDevice());

	postEffectManager_ = std::make_unique<PostEffectManager>();
	postEffectManager_->Initialize(context_->graphicsDevice->GetDevice());

	// ノイズ画像をロードしてセット
	uint32_t noiseIndex = context_->textureManager->LoadTexture("Resources/Noise/fire_noise.png");

	// PostEffectManager 経由で Dissolve にセット
	if(auto* dissolve = postEffectManager_->GetEffect<Dissolve>(PostEffectType::Dissolve)) {
		dissolve->SetMaskTextureIndex(noiseIndex);
	}
}

void PlayScene::Update(CameraData* cameraData) {
	InputManager* input = InputManager::GetInstance();
#ifdef USEIMGUI
	// Tabキーでプレイモード/デバッグモードを切り替える
	if (input->GetRawInput()->Trigger(VK_TAB)) {
		isDebugMode_ = !isDebugMode_;
		// シーン内のカメラを探して優先度（Priority）を切り替える
		for (auto& obj : gameObjects_) {
			// デバッグカメラの優先度設定
			if (auto* debugCam = obj->GetComponent<VirtualDebugCamera>()) {
				debugCam->SetPriority(isDebugMode_ ? 20 : 10);
			}
			// 追従カメラの優先度設定
			if (auto* followCam = obj->GetComponent<VirtualFollowCamera>()) {
				followCam->SetPriority(isDebugMode_ ? 10 : 20);
			}
		}
	}
	// 毎フレーム、全てのコンポーネントにデバッグ状態を通知する
	for (auto& obj : gameObjects_) {
		obj->SetIsDebugMode(isDebugMode_);
	}
#else
	// リリースビルド時は常にデバッグモードをOFF
	for (auto& obj : gameObjects_) {
		obj->SetIsDebugMode(false);
	}
#endif

	// 全オブジェクトの更新
	for (auto& obj : gameObjects_) {
		obj->Update();
	}

	// 当たり判定の実行
	CollisionManager::GetInstance()->UpdateAllCollisions();

	// ループ終了後に、追加待ちのオブジェクトをメインリストに合流させる
	if (!createQueue_.empty()) {
		for (auto& newObj : createQueue_) {
			gameObjects_.push_back(std::move(newObj));
		}
		createQueue_.clear(); // キューを空にする
	}

	// 死亡フラグが立っているオブジェクトを削除
	CleanupObject();

	// カメラの更新
	CameraOrganizer::GetInstance()->Update();

	// 最新のカメラ座標に基づいて、レティクルを更新する！
	for (auto& obj : gameObjects_) {
		if (auto* reticle = obj->GetComponent<ReticleComponent>()) {
			reticle->Update();
		}
	}

	// ライトの更新
	if (lightManager_) {
		lightManager_->ClearLights();

		// 全オブジェクトからLightComponentを探して登録
		for(auto& obj : gameObjects_) {
			if(auto* light = obj->GetComponent<LightComponent>()) {
				lightManager_->Register(light);
			}
		}

		lightManager_->ImGui();
		lightManager_->Update();
	}

#ifdef USEIMGUI
	postEffectManager_->ImGui();

	// エディタの更新処理に丸投げ！
	levelEditor_->Update(gameObjects_, selectedObject_, cameraData);
#endif
}

void PlayScene::Draw(MyEngine::Rendering::Renderer* renderer) {
	// ライトマネージャーを RenderSystem に登録
	if (lightManager_ && renderer) {
		renderer->GetRenderSystem()->SetLightManager(lightManager_.get());
	}

	renderer->Draw(gameObjects_);

	renderer->SetPostEffectManager(postEffectManager_.get());
}

void PlayScene::CleanupObject() {
	// デスフラグ（isDead_）が立っているオブジェクトをリストから削除する
	for (auto it = gameObjects_.begin(); it != gameObjects_.end();) {
		if ((*it)->IsDead()) {
			it = gameObjects_.erase(it); // メモリ解放＆削除
		} else {
			++it;
		}
	}
}
