#include "PCH.h"
#include "VirtualCameraComponent.h"
#include "GameObject.h"
#include "CameraOrganizer.h"

VirtualCameraComponent::~VirtualCameraComponent() {
	// 破棄されるときにマネージャーから登録を解除する！
	CameraOrganizer::GetInstance()->UnregisterVirtualCamera(this);
}

void VirtualCameraComponent::Initialize() {
	// 初期化時にマネージャーに自身を登録
	CameraOrganizer::GetInstance()->RegisterVirtualCamera(this);
}

void VirtualCameraComponent::ImGui() {
	ImGui::DragInt("優先度", &priority_, 1, 0, 100);
	ImGui::DragFloat("視野角", &fov_, 0.01f, 0.1f, 2.0f);
}

void VirtualCameraComponent::Serialize(json& j) const {
	j["priority"] = priority_;
	j["fov"] = fov_;
}

void VirtualCameraComponent::Deserialize(const json& j) {
	if (j.contains("priority")) priority_ = j["priority"];
	if (j.contains("fov")) fov_ = j["fov"];
}

Vector3 VirtualCameraComponent::GetPosition() const {
	return gameObject_->GetTransform().translate;
}

Vector3 VirtualCameraComponent::GetRotate() const {
	return gameObject_->GetTransform().rotate;
}
