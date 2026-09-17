#include "PCH.h"
//#include "TitleScene.h"
//#include "CameraOrganizer.h"
//#include "InputManager.h"
//#include "ModelManager.h"
//#include "TextureManager.h"
//#include "Easing.h"
//#include "imgui.h"
//#include "SceneManager.h"
//#include "SceneType.h"
//#include "AudioManager.h"
//
////デルタタイムを定義
//const float Time::GetDeltaTime() = 1.0f / 60.0f;
//
//TitleScene::TitleScene() {
//	TextureManager::GetInstance()->LoadTexture("Resources/titleScene/title.png", "title");
//	ModelManager::GetInstance()->LoadModelData("Resources/skydome", "skydome.obj");
//	TextureManager::GetInstance()->LoadTexture("Resources/skydome/skydome.png", "skydome");
//}
//
//TitleScene::~TitleScene () {
//	//リソースを解放する
//	TextureManager::GetInstance()->UnloadTexture("Resources/titleScene/title.png");
//	//ModelManager::GetInstance()->UnloadModelData("skydome.obj");
//	//TextureManager::GetInstance()->UnloadTexture("Resources/skydome/skydome.png");
//}
//
//void TitleScene::Initialize (CameraOrganizer* camera, InputManager* inputManager, DxCommon* dxCommon) {
//	camera_ = camera;
//	input_ = inputManager;
//
//	lightManager_ = std::make_unique<LightManager>(dxCommon);
//	lightManager_->Initialize();
//
//	title_ = std::make_unique<Sprite>(dxCommon);
//	title_->SetTexture("title");
//	title_->Initialize({ 200.0f, 200.0f, 0.0f });
//}
//
//void TitleScene::Update () {
//	if(input_->GetRawInput()->Trigger(VK_SPACE)) {
//		//シーン遷移時のもろもろ
//		StopToResources();
//
//		//最後にシーンを更新
//		nextScene_ = std::make_unique<PlayScene>();
//		SceneManager::GetInstance()->SetNextScene(std::move(nextScene_));
//	}
//
//	camera_->Update();
//
//	lightManager_->Update();
//	lightManager_->ImGui();
//
//	title_->Update();
//	title_->ImGui();
//}
//
//void TitleScene::Draw() {
//	title_->Draw();
//}
//
//void TitleScene::StopToResources() {
//
//}