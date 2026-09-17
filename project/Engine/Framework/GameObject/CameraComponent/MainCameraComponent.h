#pragma once
#include "Component.h"
#include "CameraOrganizer.h"

class MainCameraComponent : public Component {
public:
	MainCameraComponent() = default;
	~MainCameraComponent() override;

	void Initialize() override;
	void Update() override;
	void ImGui() override;

	// 行列の更新処理
	void UpdateMatrix();

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	// --- アクセッサ --- //
	// 名前を取得
	const char* GetName() const override { return "Camera"; }

	// カメラデータを取得
	CameraData& GetCameraData() { return cameraData_; }

	// ニアクリップを取得
	float GetNear() const { return near_; }

	// ファークリップを取得
	float GetFar() const { return far_; }

private:

private:
	CameraData cameraData_ = {};
	float near_ = 0.1f;
	float far_ = 1000.0f;
};