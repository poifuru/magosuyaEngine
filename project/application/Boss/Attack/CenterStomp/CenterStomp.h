#pragma once
#include "MagosuyaEngine.h"
#include "object/3d/Model.h"

class CenterStomp {
public:
	CenterStomp(MagosuyaEngine* magosuya, CameraData* camera);
	~CenterStomp();

	void Initialize();
	void Update();
	void Draw();
private:
	void UpdateCamera();
private:
	MagosuyaEngine* magosuya_ = nullptr;
	CameraData* camera_;
	Matrix4x4 vp_;
	std::unique_ptr<Model> model_ = nullptr;

	Transform transform_;
};

