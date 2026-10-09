#pragma once
#include "BaseLight.h"

class PointLight : public BaseLight {
public:
	PointLight();
	~PointLight() override;
	void Initialize() override;
	void Update() override;
	void ImGui(int index);

	const Vector3& GetPosition() const { return position_; }
	void SetPosition(const Vector3& pos) { position_ = pos; }
	float GetRadius() const { return radius_; }
	void SetRadius(const float& radius) { radius_ = radius; }
	float GetDecay() const { return decay_; }
	void SetDecay(const float& decay) { decay_ = decay; }

private:
	Vector3 position_;
	float radius_;	//ライトが届く最大距離
	float decay_;	//減衰率
};