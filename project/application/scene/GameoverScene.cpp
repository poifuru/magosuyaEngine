#include "PCH.h"
//#include "GameoverScene.h"
//#include "MagosuyaEngine.h"
//#include "InputManager.h"
//#include "Easing.h"
//#include "SceneManager.h"
//#include "SceneType.h"
//
////デルタタイムを定義
//const float Time::GetDeltaTime() = 1.0f / 60.0f;
//
//GameoverScene::GameoverScene () {
//	TextureManager::GetInstance()->LoadTexture("Resources/gameover.png", "gameover");
//	ModelManager::GetInstance()->LoadModelData("Resources/skydome", "skydome.obj");
//	TextureManager::GetInstance()->LoadTexture("Resources/skydome/skydome.png", "skydome");
//}
//
//GameoverScene::~GameoverScene () {
//	TextureManager::GetInstance()->UnloadTexture("Resources/gameover.png");
//	//ModelManager::GetInstance()->UnloadModelData("skydome.obj");
//	//TextureManager::GetInstance()->UnloadTexture("Resources/skydome/skydome.png");
//}
//
//void GameoverScene::Initialize (CameraOrganizer* camera, InputManager* inputManager, DxCommon* dxCommon) {
//	camera_ = camera;
//	input_ = inputManager;
//
//	lightManager_ = std::make_unique<LightManager>(dxCommon);
//	lightManager_->Initialize();
//
//	camera_->AddCamera ("main3", CameraType::FixedPointCamera);
//	camera_->SetActiveCamera("main3");
//	camera_->SetPosition({ 0.0f, 100.0f, -10.0f });
//
//	gameover_ = std::make_unique<Sprite>(dxCommon);
//	gameover_->SetTexture("gameover");
//	gameover_->Initialize({ 0.0f, 0.0f, 0.0f });
//
//	skydome_ = std::make_unique<Model>(dxCommon, lightManager_.get());
//	skydome_->SetModelData("skydome.obj");
//	skydome_->SetTexture("skydome");
//	skydome_->Initialize();
//	skydome_->IsLighting(LightReflectionModel::None);
//}
//
//void GameoverScene::Update () {
//	if(input_->GetRawInput()->Trigger(VK_SPACE)) {
//		nextScene_ = std::make_unique<TitleScene>();
//		SceneManager::GetInstance()->SetNextScene(std::move(nextScene_));
//	}
//
//	camera_->Update();
//
//	lightManager_->Update();
//	lightManager_->ImGui();
//
//	gameover_->Update();
//
//	skydome_->Update(&camera_->GetCameraData());
//}
//
//void GameoverScene::Draw () {
//	skydome_->Draw();
//	gameover_->Draw();
//}
//
//void GameoverScene::StopToResources() {
//
//}
