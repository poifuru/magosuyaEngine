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
	void IsHit (bool attackIsHit);

private:
	void AABBPos ();

public:
	AABB GetAABBModel () { return aabb_model_; }
	bool GetIsHit () { return isHit_; }
	int GetHp () { return hp_; }
	
private:
	std::unique_ptr<Model> model_ = nullptr;
	Transform transform_ = {};
	AABB aabb_model_ = {};

	//固有ステ
	int hp_ = 0;
	bool isHit_ = false;

	//ポインタを借りる
	MagosuyaEngine* magosuya_ = nullptr;
	CameraData* cameraData_ = nullptr;
};

