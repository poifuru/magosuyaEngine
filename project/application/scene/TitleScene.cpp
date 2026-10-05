#include "PCH.h"
#include "TitleScene.h"
#include "PlayScene.h"
#include "SceneManager.h"
#include "Renderer.h"
#include "RenderSystem.h"
#include "CameraOrganizer.h"
#include "EditorManager.h"
#include "ComponentType.h"
#include "InputManager.h"
#include "RawInput.h"
#include "CollisionManager.h"
#include "LightManager.h"
#include "PostEffectManager.h"
#include "GraphicsDevice.h"

TitleScene::TitleScene() = default;

TitleScene::~TitleScene() = default;

void TitleScene::Initialize() {
	if (!context_) return;

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

	// コンテキストにオブジェクトリストを登録
	context_->gameObjects = &createQueue_;
	context_->activeGameObjects = &gameObjects_;

	// ライトの生成
	lightManager_ = std::make_unique<LightManager>();
	lightManager_->Initialize(context_->graphicsDevice->GetDevice());

	// ポストエフェクトの生成
	postEffectManager_ = std::make_unique<PostEffectManager>();
	postEffectManager_->Initialize(context_->graphicsDevice->GetDevice());

	// titleScene.json があれば読み込む
	const std::string scenePath = "Resources/Scene/titleScene.json";
	if (std::filesystem::exists(scenePath)) {
		std::ifstream file(scenePath);
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
			}
		}
	}

#ifdef USEIMGUI
	// エディタにタイトルシーンのコンテキストを教えてあげる
	EditorManager::GetInstance()->SetSceneContext(context_);
#endif
}

void TitleScene::UpdateGame(CameraData* cameraData) {
	// スペースキーまたはゲームパッドのボタンでインゲームへ遷移
	auto* rawInput = InputManager::GetInstance()->GetRawInput();
	if (rawInput->Trigger(VK_SPACE)) {
		if (context_->sceneManager) {
			// 1.5秒かけてシームレスにPlaySceneへ
			context_->sceneManager->ChangeScene<PlayScene>(1.5f);
		}
	}

	// 予約されたオブジェクトの追加
	if (!createQueue_.empty()) {
		for (auto& newObj : createQueue_) {
			gameObjects_.push_back(std::move(newObj));
		}
		createQueue_.clear();
	}

	// 各オブジェクトの更新
	for (auto& obj : gameObjects_) {
		obj->Update();
	}

	CleanupObject();

	CameraOrganizer::GetInstance()->Update();

	if (lightManager_) {
		lightManager_->Update();
	}
}

void TitleScene::UpdateEdit(CameraData* cameraData) {
	// エディタ編集中の更新処理
	if (!createQueue_.empty()) {
		for (auto& newObj : createQueue_) {
			gameObjects_.push_back(std::move(newObj));
		}
		createQueue_.clear();
	}
	CameraOrganizer::GetInstance()->Update();

	for (auto& obj : gameObjects_) {
		obj->Update();
	}

	// 停止中でもデバッグカメラをマウス等で動かせるようにする
	for (auto& obj : gameObjects_) {
		if (auto* debugCam = obj->GetComponent<VirtualDebugCamera>()) {
			debugCam->Update();
		}
	}
	// カメラ管理の更新
	CameraOrganizer::GetInstance()->Update();

	CleanupObject();
}

void TitleScene::Draw(MyEngine::Rendering::Renderer* renderer) {
	// ライトマネージャーを RenderSystem に登録
	if (lightManager_ && renderer) {
		renderer->GetRenderSystem()->SetLightManager(lightManager_.get());
	}

	renderer->Draw(gameObjects_);

	renderer->SetPostEffectManager(postEffectManager_.get());
}

void TitleScene::CleanupObject() {
	for (auto it = gameObjects_.begin(); it != gameObjects_.end(); ) {
		if ((*it)->IsDead()) {
			it = gameObjects_.erase(it);
		} else {
			++it;
		}
	}
}