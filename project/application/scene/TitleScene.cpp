#include "TitleScene.h"
#include "MagosuyaEngine.h"
#include "utility/input/InputManager.h"
#include "MathFunction.h"
#include <imgui.h>

TitleScene::TitleScene (MagosuyaEngine* magosuya) {
	magosuya_ = magosuya;
	camera_ = std::make_unique<CameraData> ();
	model_ = std::make_unique<Model> (magosuya);
	magosuya_->LoadModelData ("Resources/teapot", "teapot");
	player_ = std::make_unique<Player>(magosuya);
}

TitleScene::~TitleScene () {

}

void TitleScene::Initialize () {
	nowScene_ = SceneLabel::Title;
	isFinish_ = false;

	model_->SetModelData ("teapot");
	model_->SetTexture ("teapot");
	model_->Initialize ();

	camera_->transform = {
		{1.0f, 1.0f, 1.0f},
		{},
		{0.0f, 0.0f, -50.0f},
	};

	player_->Initialize();
}

void TitleScene::Update () {
	//カメラ
	camera_->world = MakeAffineMatrix (camera_->transform.scale, camera_->transform.rotate, camera_->transform.translate);
	camera_->view = Inverse (camera_->world);
	camera_->proj = MakePerspectiveFOVMatrix (
		0.45f, float (magosuya_->GetDxCommon ()->GetWinAPI ()->kClientWidth) / float (magosuya_->GetDxCommon ()->GetWinAPI ()->kClientHeight), 0.1f, 100.0f
	);
	Matrix4x4 vp = Multiply (camera_->view, camera_->proj);

	model_->Update (&vp);
	player_->Update(&vp);
}

void TitleScene::Draw () {
	model_->Draw ();
	player_->Draw();
}