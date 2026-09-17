#include "PCH.h"
#include "MainCameraComponent.h"
#include "CameraOrganizer.h"
#include "GameObject.h"
#include "MathFunction.h"
#include "WindowsAPI.h"

MainCameraComponent::~MainCameraComponent() {
	CameraOrganizer::GetInstance()->UnregisterMainCamera(this);
}

void MainCameraComponent::Initialize() {
	// マネージャに実体カメラとして登録
	CameraOrganizer::GetInstance()->RegisterMainCamera(this);
}

void MainCameraComponent::Update() {
	// 自身のTransform情報を反映して、ビュー・プロジェクション行列を計算する
	UpdateMatrix();
}

void MainCameraComponent::ImGui() {
	ImGui::DragFloat("Near Clip", &near_, 0.1f, 0.01f, 10.0f);
	ImGui::DragFloat("Far Clip", &far_, 10.0f, 10.0f, 5000.0f);
}

void MainCameraComponent::UpdateMatrix() {
	auto& trans = gameObject_->GetTransform();
	cameraData_.transform = trans;
	// ワールド行列の作成
	cameraData_.world = Math::MakeAffineMatrix(trans.scale, trans.rotate, trans.translate);
	// ビュー行列（ワールドの逆行列）
	cameraData_.view = Math::Inverse(cameraData_.world);

	// プロジェクション行列
	float aspectRatio = WindowsAPI::GetInstance()->GetAspectRatio();
	cameraData_.proj = Math::MakePerspectiveFOVMatrix(CameraOrganizer::GetInstance()->GetActiveFov(), aspectRatio, near_, far_);

	// VP行列
	cameraData_.vp = Math::Multiply(cameraData_.view, cameraData_.proj);
}

void MainCameraComponent::Serialize(json& j) const {
	j["type"] = "MainCameraComponent";
	j["near"] = near_;
	j["far"] = far_;
}

void MainCameraComponent::Deserialize(const json& j) {
	if (j.contains("near")) near_ = j["near"];
	if (j.contains("far")) far_ = j["far"];
}