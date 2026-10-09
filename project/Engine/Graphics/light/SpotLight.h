#pragma once
#include "BaseLight.h"

class SpotLight : public BaseLight {
public:
	SpotLight();
	~SpotLight() override;
	void Initialize() override;
	void Update() override;
	void ImGui(int index);

	const Vector3& GetPosition() const { return position_; }
	void SetPosition(const Vector3& pos) { position_ = pos; }
	const Vector3& GetDirection() const { return direction_; }
	void SetDirection(const Vector3& dir) { direction_ = dir; }
	float GetDistance() const { return distance_; }
	void SetDistance(const float& dis) { distance_ = dis; }
	float GetDecay() const { return decay_; }
	void SetDecay(const float& decay) { decay_ = decay; }
	float GetCosAngle() const { return cosAngle_; }
	void SetCosAngle(const float& cosAngle) { cosAngle_ = cosAngle; }

private:
	Vector3 position_;
	Vector3 direction_;
	float distance_;	//ライトが届く最大	距離
	float decay_;	//減衰率
	float cosAngle_;	//スポットライトの余弦
};

