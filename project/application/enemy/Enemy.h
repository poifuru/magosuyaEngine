#pragma once
#include <memory>
#include "../../general/struct.h"
#include "../../magosuya/object/3d/Model.h"

class Enemy {
public:		//メンバ関数
	Enemy (MagosuyaEngine* magosuya, CameraData* cameraData);
	~Enemy ();

	void Initialize ();
	void Update ();
	void Draw ();
	
private:
	std::unique_ptr<Model> model_ = nullptr;
	Transform transform_ = {};

	//ポインタを借りる
	MagosuyaEngine* magosuya_ = nullptr;
	CameraData* cameraData_ = nullptr;
};

