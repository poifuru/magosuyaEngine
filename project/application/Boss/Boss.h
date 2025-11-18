#pragma once
#include "../../magosuya/MagosuyaEngine.h"
#include "object/3d/Model.h"

class Boss {
public:
	Boss(MagosuyaEngine* magosuya, CameraData* camera);
	~Boss();

	void Initialize();
	void Update();
	void Draw();

	Transform& GetTransform() { return position_; }
	Vector3& GetPosition() { return position_.translate; }

private:
	void UpdateCamera();
	void UpdateMove();
private:
	MagosuyaEngine* magosuya_ = nullptr;
	CameraData* camera_;
	Matrix4x4 vp_;
	std::unique_ptr<Model> model_ = nullptr;
	
	Transform position_;
	Vector3 speed_;
};

