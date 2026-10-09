#pragma once
#include "BaseLight.h"

class RectLight : public BaseLight {
public:
	RectLight();
	~RectLight() override;
	void Initialize() override;
	void Update() override;
	void ImGui(int index);

	const Vector3& GetPosition() const { return position_; }
	void SetPosition(const Vector3& pos) { position_ = pos; }
	const Vector3& GetDirection() const { return direction_; }
	void SetDirection(const Vector3& dir) { direction_ = dir; }
	const Vector2& GetSize() const { return size_; }
	void SetSize(const Vector2& size) { size_ = size; }
	const Vector3& GetRight() const { return right_; }
	void SetRight(const Vector3& right) { right_ = right; }
	const Vector3& GetUp() const { return up_; }
	void SetUp(const Vector3& up) { up_ = up; }
	float GetDecay() const { return decay_; }
	void SetDecay(const float& decay) { decay_ = decay; }

private:
	Vector3 position_;
	Vector3 direction_;
	Vector2 size_;
	Vector3 right_;
	Vector3 up_;
	float decay_;
};

