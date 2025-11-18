#include "CenterStomp.h"
#include "MathFunction.h"
#include <imgui.h>

CenterStomp::CenterStomp(MagosuyaEngine* magosuya, CameraData* camera) {
	magosuya_ = magosuya;
	camera_ = camera;
	model_ = std::make_unique<Model>(magosuya);
	magosuya_->LoadModelData("Resources/teapot", "teapot");
}

CenterStomp::~CenterStomp() {

}

void CenterStomp::Initialize() {
	model_->SetModelData("teapot");
	model_->SetTexture("teapot");
	model_->Initialize();

	transform_ = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
}

void CenterStomp::Update() {
	// カメラの更新
	UpdateCamera();

	model_->SetTransform(transform_);
	model_->Update(&vp_);
}

void CenterStomp::Draw() {
	model_->Draw();
}

void CenterStomp::UpdateCamera() {
	camera_->world = MakeAffineMatrix(camera_->transform.scale, camera_->transform.rotate, camera_->transform.translate);
	camera_->view = Inverse(camera_->world);
	camera_->proj = MakePerspectiveFOVMatrix(
		0.45f, float(magosuya_->GetDxCommon()->GetWinAPI()->kClientWidth) / float(magosuya_->GetDxCommon()->GetWinAPI()->kClientHeight), 0.1f, 100.0f
	);
	vp_ = Multiply(camera_->view, camera_->proj);
}