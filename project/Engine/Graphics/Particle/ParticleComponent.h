#pragma once
#include "Component.h"
#include "MathFunction.h"

class ParticleComponent : public Component {
public:
	ParticleComponent() = default;
	~ParticleComponent() override = default;

	void Initialize() override;
	void Update() override;

	const char* GetName() const override { return "ParticleComponent"; }

	// パラメータ設定用
	void SetVelocity(const Vector3& vel) { velocity_ = vel; }
	void SetGravity(float gravity) { gravity_ = gravity; }
	void SetLifeTime(float lifeTime) { lifeTime_ = lifeTime; maxLifeTime_ = lifeTime; }
	void SetColorFade(const Vector4& startColor, const Vector4& endColor) {
		startColor_ = startColor;
		endColor_ = endColor;
		hasColorFade_ = true;
	}

private:
	Vector3 velocity_{ 0.0f, 0.0f, 0.0f }; // 移動速度
	float gravity_ = 0.0f;                 // 下方向への重力加速度
	float maxLifeTime_ = 1.0f;             // 最大寿命
	float lifeTime_ = 1.0f;                // 残り寿命

	bool hasColorFade_ = false;            // 色フェードを行うか
	Vector4 startColor_{ 1.0f, 1.0f, 1.0f, 1.0f };
	Vector4 endColor_{ 1.0f, 1.0f, 1.0f, 0.0f };
};
