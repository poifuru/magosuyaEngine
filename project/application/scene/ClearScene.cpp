#include "PCH.h"
//#include "ClearScene.h"
//#include "Easing.h"
//#include "ModelManager.h"
//#include "TextureManager.h"
//#include "SceneManager.h"
//#include "SceneType.h"
//
////デルタタイムを定義
//const float Time::GetDeltaTime() = 1.0f / 60.0f;
//
//ClearScene::ClearScene () {
//	TextureManager::GetInstance()->LoadTexture("Resources/clear.png", "clear");
//	//ModelManager::GetInstance()->LoadModelData("Resources/skydome", "skydome.obj");
//	//TextureManager::GetInstance()->LoadTexture("Resources/skydome/skydome.png", "skydome");
//}
//
//ClearScene::~ClearScene () {
//	TextureManager::GetInstance()->UnloadTexture("Resources/clear.png");
//	//ModelManager::GetInstance()->UnloadModelData("skydome.obj");
//	//TextureManager::GetInstance()->UnloadTexture("Resources/skydome/skydome.png");
//}
//
//void ClearScene::Initialize (CameraOrganizer* camera, InputManager* inputManager, DxCommon* dxCommon) {
//	camera_ = camera;
//	input_ = inputManager;
//
//	lightManager_ = std::make_unique<LightManager>(dxCommon);
//	lightManager_->Initialize();
//
//	camera_->AddCamera ("main2", CameraType::FixedPointCamera);
//	camera_->SetActiveCamera("main2");
//	camera_->SetPosition({ 0.0f, 100.0f, -10.0f });
//
//	clear_ = std::make_unique<Sprite>(dxCommon);
//	clear_->SetTexture("clear");
//	clear_->Initialize({ 0.0f, 0.0f, 0.0f });
//
//	skydome_ = std::make_unique<Model>(dxCommon, lightManager_.get());
//	skydome_->SetModelData("skydome.obj");
//	skydome_->SetTexture("skydome");
//	skydome_->Initialize();
//	skydome_->IsLighting(LightReflectionModel::None);
//}
//
//void ClearScene::Update () {
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
//	clear_->Update();
//
//	skydome_->Update(&camera_->GetCameraData());
//}
//
//void ClearScene::Draw () {
//	skydome_->Draw();
//	clear_->Draw();
//}
//
//void ClearScene::StopToResources() {
//
//}
