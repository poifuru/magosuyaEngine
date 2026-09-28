#pragma once
#include "MathTypes.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class GameObject;

class PlayerTurret {
public:
	PlayerTurret() = default;
	~PlayerTurret() = default;

	void Initialize();
	void Update(GameObject* gameObject, GameObject* reticleObject);

	void ImGui();
	void Serialize(json& j) const;
	void Deserialize(const json& j);

private:
	// 大砲（Canonノード）の調整用パラメータ
	Quaternion initialCanonRotation_{ 0.0f, 0.0f, 0.0f, 1.0f };
	bool hasCapturedCanonInitialRot_ = false;
	float canonOffsetYaw_ = 0.0f;   // 左右オフセット（度）
	float canonOffsetPitch_ = 0.0f; // 上下オフセット（度）
};
