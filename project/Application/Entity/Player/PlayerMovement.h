#pragma once
#include "MathTypes.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class GameObject;

class PlayerMovement {
public:
	PlayerMovement() = default;
	~PlayerMovement() = default;

	void Initialize();
	void Update(GameObject* gameObject);

	void ImGui();
	void Serialize(json& j) const;
	void Deserialize(const json& j);

	// ゲッター・セッター
	const Vector3& GetForward() const { return forward_; }
	const Vector3& GetVelocity() const { return velocity_; }
	void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }

private:
	void Move(GameObject* gameObject);

private:
	// 移動パラメータ
	float speed_ = 0.5f;
	float maxSpeed_ = 2.5f;             // 最高速度
	float attenuationRate_ = 0.98f;     // 速度の減衰率(慣性)
	float brakeAttenuationRate_ = 0.90f;// ブレーキ時の減衰（値が小さいほど急制動）
	Vector3 velocity_{};
	Vector3 acceleration_{};
	Vector3 forward_{};
	float turnSpeed_ = 0.3f;            // 左右への回転を補間するスピード
	float dirRatioZ_ = 0.97f;           // 曲がるときにどのくらいの比率を掛けるか(前方)
	float dirRatioX_ = 0.03f;           // 曲がるときにどのくらいの比率を掛けるか(左右)

	// ブレーキ時の挙動トレードオフ倍率
	float brakeTurnSpeedMultiplier_ = 3.0f; // ブレーキ中の旋回力倍率
	float brakeMoveSpeedMultiplier_ = 0.2f; // ブレーキ中の移動力（加速）倍率
};
