#include "Boss.h"
#include "MathFunction.h"
#include <imgui.h>

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

	centerStomp_ = std::make_unique<CenterStomp>(magosuya_, camera_);
	centerStomp_->Initialize();

	transform_ = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
	speed_ = { 0.1f,0.1f,0.1f };
}

void Boss::Update() {
	// カメラの更新
	UpdateCamera();
	// 行動の更新
	UpdateMove();


	model_->Update(&vp_);
	centerStomp_->Update();
}

void Boss::Draw() {
	model_->Draw();
	centerStomp_->Draw();
}

void Boss::ImGuiControl() {
#ifdef _DEBUG
	ImGui::Begin("Boss");
	ImGui::DragFloat3("Scale", &transform_.scale.x, 0.1f);
	ImGui::DragFloat3("Rotate", &transform_.rotate.x, 0.1f);
	ImGui::DragFloat3("Translate", &transform_.translate.x, 0.1f);
	ImGui::End();
#endif
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
		transform_.translate.z += speed_.z;
	}
	if (magosuya_->GetRawInput()->Push(VK_DOWN)) {
		transform_.translate.z -= speed_.z;
	}
	if (magosuya_->GetRawInput()->Push(VK_LEFT)) {
		transform_.translate.x -= speed_.x;
	}
	if (magosuya_->GetRawInput()->Push(VK_RIGHT)) {
		transform_.translate.x += speed_.x;
	}
	model_->SetTransform(transform_);
}