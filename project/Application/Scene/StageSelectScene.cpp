#include "PCH.h"
#include "StageSelectScene.h"
#include "SceneManager.h"
#include "Renderer.h"
#include "RenderSystem.h"
#include "CameraOrganizer.h"
#include "EditorManager.h"
#include "ComponentType.h"
#include "LightManager.h"
#include "PostEffectManager.h"
#include "GraphicsDevice.h"
#include "Dissolve.h"
#include "StartupSceneSettings.h"

namespace {
	// ステージセレクト画面のシーンファイル（背景・カメラ・文字の配置はエディタで編集できる）
	const std::string kScenePath = "Resources/Scene/stageSelectScene.json";
}

StageSelectScene::StageSelectScene() = default;

StageSelectScene::~StageSelectScene() = default;

void StageSelectScene::Initialize() {
	if (!context_) return;

	// コンテキストにオブジェクトリストを登録
	context_->gameObjects = &createQueue_;
	context_->activeGameObjects = &gameObjects_;

	// ライトの生成
	lightManager_ = std::make_unique<LightManager>();
	lightManager_->Initialize(context_->graphicsDevice->GetDevice());

	// ポストエフェクトの生成
	postEffectManager_ = std::make_unique<PostEffectManager>();
	postEffectManager_->Initialize(context_->graphicsDevice->GetDevice());

	// ディゾルブ用テクスチャをセット
	uint32_t noiseIndex = context_->textureManager->LoadTexture("Resources/noise0.png");
	if (auto* dissolve = postEffectManager_->GetEffect<Dissolve>(PostEffectType::Dissolve)) {
		dissolve->SetMaskTextureIndex(noiseIndex);
	}

	// エディタの保存先をこのシーンのファイルにする（設定しないと直前に開いた PlayScene のファイルに保存されてしまう）
	StartupSceneSettings::SetCurrentScenePath(kScenePath);

	// stageSelectScene.json を読み込む
	if (std::filesystem::exists(kScenePath)) {
		std::ifstream file(kScenePath);
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
	else {
		Logger::GetInstance()->Log("StageSelectScene: " + kScenePath + " が見つかりません");
	}

	// --- カメラの初期優先度設定（TitleScene と同じ） ---
	for (auto& obj : gameObjects_) {
		if (auto* debugCam = obj->GetComponent<VirtualDebugCamera>()) {
#ifdef USEIMGUI
			debugCam->SetPriority(20);
#else
			debugCam->SetPriority(10);
#endif
		}
		if (auto* followCam = obj->GetComponent<VirtualFollowCamera>()) {
#ifdef USEIMGUI
			followCam->SetPriority(10);
#else
			followCam->SetPriority(20);
#endif
		}
	}

#ifdef USEIMGUI
	EditorManager::GetInstance()->SetSceneContext(context_);
#endif
}

void StageSelectScene::UpdateGame(const CameraData* cameraData) {
	// 予約されたオブジェクトの追加
	if (!createQueue_.empty()) {
		for (auto& newObj : createQueue_) {
			gameObjects_.push_back(std::move(newObj));
		}
		createQueue_.clear();
	}

	// 各オブジェクトの更新（カーソル移動・決定は StageSelectComponent の Update）
	for (auto& obj : gameObjects_) {
		obj->Update();
	}

	CleanupObject();

	CameraOrganizer::GetInstance()->Update();
	UpdateLights();
}

void StageSelectScene::UpdateEdit(const CameraData* cameraData) {
	if (!createQueue_.empty()) {
		for (auto& newObj : createQueue_) {
			gameObjects_.push_back(std::move(newObj));
		}
		createQueue_.clear();
	}

	// 停止中も文字の表示は更新したいので Update は呼ぶ（入力は StageSelectComponent 側で無視する）
	for (auto& obj : gameObjects_) {
		obj->Update();
	}

	// 停止中でもデバッグカメラをマウス等で動かせるようにする
	for (auto& obj : gameObjects_) {
		if (auto* debugCam = obj->GetComponent<VirtualDebugCamera>()) {
			debugCam->Update();
		}
	}
	CameraOrganizer::GetInstance()->Update();

	CleanupObject();
	UpdateLights();
}

void StageSelectScene::Draw(MyEngine::Rendering::Renderer* renderer) {
	if (lightManager_ && renderer) {
		renderer->GetRenderSystem()->SetLightManager(lightManager_.get());
	}

	renderer->Draw(gameObjects_);

	renderer->SetPostEffectManager(postEffectManager_.get());
}

void StageSelectScene::CleanupObject() {
	for (auto it = gameObjects_.begin(); it != gameObjects_.end(); ) {
		if ((*it)->IsDead()) {
			it = gameObjects_.erase(it);
		} else {
			++it;
		}
	}
}

void StageSelectScene::UpdateLights() {
	// PlayScene と同じく、シーン内の LightComponent を毎フレーム登録し直す
	if (!lightManager_) return;
	lightManager_->ClearLights();
	for (auto& obj : gameObjects_) {
		if (auto* light = obj->GetComponent<LightComponent>()) {
			lightManager_->Register(light);
		}
	}
	lightManager_->Update();
}
