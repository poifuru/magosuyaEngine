#include "Boss.h"
#include "MathFunction.h"

Boss::Boss(MagosuyaEngine* magosuya, CameraData* camera) {
	magosuya_ = magosuya;
	camera_ = camera;
	model_ = std::make_unique<Model>(magosuya);
	magosuya_->LoadModelData("Resources/teapot", "teapot");
}

Boss::~Boss() {

}

void Boss::Initialize() {
	model_->SetModelData("teapot");
	model_->SetTexture("teapot");
	model_->Initialize();

	position_ = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
	speed_ = { 0.1f,0.1f,0.1f };
}

void Boss::Update() {
	// カメラの更新
	UpdateCamera();
	// 行動の更新
	UpdateMove();


	model_->Update(&vp_);
}

void Boss::Draw() {
	model_->Draw();
}

void Boss::UpdateCamera() {
	camera_->world = MakeAffineMatrix(camera_->transform.scale, camera_->transform.rotate, camera_->transform.translate);
	camera_->view = Inverse(camera_->world);
	camera_->proj = MakePerspectiveFOVMatrix(
		0.45f, float(magosuya_->GetDxCommon()->GetWinAPI()->kClientWidth) / float(magosuya_->GetDxCommon()->GetWinAPI()->kClientHeight), 0.1f, 100.0f
	);
	vp_ = Multiply(camera_->view, camera_->proj);
}

void Boss::UpdateMove() {
	if (magosuya_->GetRawInput()->Push(VK_UP)) {
		position_.translate.z += speed_.z;
	}
	if (magosuya_->GetRawInput()->Push(VK_DOWN)) {
		position_.translate.z -= speed_.z;
	}
	if (magosuya_->GetRawInput()->Push(VK_LEFT)) {
		position_.translate.x -= speed_.x;
	}
	if (magosuya_->GetRawInput()->Push(VK_RIGHT)) {
		position_.translate.x += speed_.x;
	}
	model_->SetTransform(position_);
}