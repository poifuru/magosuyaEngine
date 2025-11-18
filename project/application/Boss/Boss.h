#pragma once
#include "MagosuyaEngine.h"
#include "object/3d/Model.h"
#include "Attack/CenterStomp/CenterStomp.h"

class Boss {
public:
	Boss(MagosuyaEngine* magosuya, CameraData* camera);
	~Boss();

	void Initialize();
	void Update();
	void Draw();
	void ImGuiControl();

	// Getter
	Transform& GetTransform() { return transform_; }
	Vector3& GetPosition() { return transform_.translate; }
	// Setter
	void SetTransform(Transform transform) { transform_ = transform; }

private:
	void UpdateCamera();
	void UpdateMove();
private:
	MagosuyaEngine* magosuya_ = nullptr;
	CameraData* camera_;
	Matrix4x4 vp_;
	std::unique_ptr<Model> model_ = nullptr;
	
	std::unique_ptr <CenterStomp> centerStomp_ = nullptr;

	Transform transform_;
	Vector3 speed_;
};

