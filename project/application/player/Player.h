#pragma once
#include <memory>
#include "../../general/struct.h"
#include "../../magosuya/object/3d/Model.h"

class MagosuyaEngine;
class InputManager;
extern std::unique_ptr<InputManager> g_inputManager;

enum Direction {
	Left,
	Right
};

class Player {
public:		//メンバ関数
	Player (MagosuyaEngine* magosuya, CameraData* cameraData);
	~Player ();

	void Initialize ();
	void Update ();
	void Draw ();

private:
	void move ();
	void Jump ();
	void Avoid ();
	void Attack ();
	void WeaponTransform ();

public:
	//アクセッサ
	int GetHp () { return hp_; }

private:	//メンバ変数
	std::unique_ptr<Model> model_ = nullptr;
	std::unique_ptr<Model> weapon_ = nullptr;
	Transform transform_ = {};
	Transform transformWep_ = {};
	Vector3 velocity_ = {};
	Direction dir_ = Direction::Right;
	int hp_ = 0;
	bool isAir_ = false;
	bool airJump_ = false;
	bool isAvoid_ = false;
	float avoidTimer_ = 0.0f;
	bool coolTime_ = false;
	float avoidCoolTime_ = 0.0f;
	bool isAttack_ = false;
	float attackTime_ = 0.0f;

	//ポインタを借りる
	MagosuyaEngine* magosuya_ = nullptr;
	CameraData* cameraData_ = nullptr;
};

