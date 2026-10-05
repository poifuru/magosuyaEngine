#include "PCH.h"
#include "PlayScene.h"
#include "Renderer.h"
#include "RenderSystem.h"
#include "CameraOrganizer.h"
#include "EditorManager.h"
#include "ComponentType.h"
#include "InputManager.h"
#include "RawInput.h"
#include "CollisionManager.h"
#include "GraphicsDevice.h"
#include "LightManager.h"
#include "PostEffectManager.h"
#include "Dissolve.h"
#include "TextureManager.h"
#include "Dissolve.h"
#include "TitleScene.h"
#include "SceneManager.h"

PlayScene::PlayScene() = default;
PlayScene::~PlayScene() = default;

void PlayScene::Initialize() {
	if (!context_) return;

	// コンテキストにリストのポインタをセットする
	context_->gameObjects = &createQueue_;

	// 本番の生存リストをセット
	context_->activeGameObjects = &gameObjects_;

	// 起動時に defaultScene.json があれば読み込む
	const std::string defaultScenePath = "Resources/Scene/defaultScene.json";
	if (std::filesystem::exists(defaultScenePath)) {
		std::ifstream file(defaultScenePath);
		if (file.is_open()) {
			nlohmann::json sceneJ;
			file >> sceneJ;
			if (sceneJ.contains("objects")) {
				for (const auto& objJ : sceneJ["objects"]) {
					auto newObj = std::make_unique<GameObject>(context_, objJ["name"]);
					newObj->Deserialize(objJ);
					newObj->Initialize();
					gameObjects_.push_back(std::move(newObj));
				}
				for (auto& obj : gameObjects_) {
					if (auto* followCam = obj->GetComponent<VirtualFollowCamera>()) {
						followCam->ResolveTarget(gameObjects_);
					}
					if (auto* player = obj->GetComponent<PlayerComponent>()) {
						player->ResolveReticle(gameObjects_);
					}
				}
			}
		}
	}

	// --- カメラの初期優先度設定 ---
	bool isPlayingMode = true;
#ifdef USEIMGUI
	// エディタがある場合：Play中ならtrue、停止中ならfalse
	isPlayingMode = EditorManager::GetInstance()->IsPlaying();
#endif
	for (auto& obj : gameObjects_) {
		// 追従カメラ
		if (auto* followCam = obj->GetComponent<VirtualFollowCamera>()) {
			followCam->SetPriority(isPlayingMode ? 20 : 10);
		}
		// デバッグカメラ
		if (auto* debugCam = obj->GetComponent<VirtualDebugCamera>()) {
			debugCam->SetPriority(isPlayingMode ? 10 : 20);
		}
	}

	// ライトマネージャーの初期化
	lightManager_ = std::make_unique<LightManager>();
	lightManager_->Initialize(context_->graphicsDevice->GetDevice());

	postEffectManager_ = std::make_unique<PostEffectManager>();
	postEffectManager_->Initialize(context_->graphicsDevice->GetDevice());

	// ディゾルブ用テクスチャをセット
	uint32_t noiseIndex = context_->textureManager->LoadTexture("Resources/noise0.png");
	if (auto* dissolve = postEffectManager_->GetEffect<Dissolve>(PostEffectType::Dissolve)) {
		dissolve->SetMaskTextureIndex(noiseIndex);
	}

	// EditorManagerにContextをセット
	EditorManager::GetInstance()->SetSceneContext(context_);
}

void PlayScene::UpdateGame(CameraData* cameraData) {
	auto* rawInput = InputManager::GetInstance()->GetRawInput();
	// ESCキー（またはゲームクリア時）にタイトルへ戻る
	if (rawInput->Trigger(VK_ESCAPE)) {
		if (context_->sceneManager && !context_->sceneManager->isTransitioning()) {
			context_->sceneManager->ChangeSceneWithDissolve<TitleScene>(0.8f, 0.8f);
		}
	}

	for (auto& obj : gameObjects_) {
		obj->Update();
	}
	CollisionManager::GetInstance()->UpdateAllCollisions();

	// 追加待ちオブジェクトの合流や死亡削除など
	if (!createQueue_.empty()) {
		for (auto& newObj : createQueue_) {
			gameObjects_.push_back(std::move(newObj));
		}
		createQueue_.clear();
	}

	// 死亡オブジェクトの削除
	CleanupObject();

	// カメラ・レティクル・ライト
	CameraOrganizer::GetInstance()->Update();
	for (auto& obj : gameObjects_) {
		if (auto* reticle = obj->GetComponent<ReticleComponent>()) {
			reticle->Update();
		}
	}
	if (lightManager_) {
		lightManager_->ClearLights();
		for (auto& obj : gameObjects_) {
			if (auto* light = obj->GetComponent<LightComponent>()) {
				lightManager_->Register(light);
			}
		}
		lightManager_->Update();
	}
}

void PlayScene::UpdateEdit(CameraData* cameraData) {
	// カメラ（デバッグカメラ）を動かす
	for (auto& obj : gameObjects_) {
		if (auto* debugCam = obj->GetComponent<VirtualDebugCamera>()) {
			debugCam->Update(); // 停止中でもマウスでカメラを動かせるようにする
		}
	}
	CameraOrganizer::GetInstance()->Update();

	// ギズモで動かした座標を画面に反映させるため、Transformの行列バッファだけ更新する
	for (auto& obj : gameObjects_) {
		obj->UpdateTransformBuffer();
	}

	// ライトの更新
	if (lightManager_) {
		lightManager_->ClearLights();
		for (auto& obj : gameObjects_) {
			if (auto* light = obj->GetComponent<LightComponent>()) {
				lightManager_->Register(light);
			}
		}
		lightManager_->Update();
	}
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