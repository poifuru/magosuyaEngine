#pragma once
#include "Collider.h"

class AttackCollider
	:public Collider
{
public:
	const Vector3 GetWorldPosition()override { return worldPosition_; }
	void SetWorldPosition(const Vector3& pos) { worldPosition_ = pos; }
private:
	Vector3 worldPosition_ = { 0.0f,0.0f,0.0f };
};

