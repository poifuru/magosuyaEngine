#pragma once
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class GameObject;

class PlayerWeapon {
public:
	PlayerWeapon() = default;
	~PlayerWeapon() = default;

	void Initialize();
	void Update(GameObject* gameObject, GameObject* reticleObject);

	void ImGui();
	void Serialize(json& j) const;
	void Deserialize(const json& j);

	// ゲッター・セッター
	float GetHarpoonMaxDistance() const { return harpoonMaxDistance_; }

private:
	void Shoot(GameObject* gameObject, GameObject* reticleObject);

private:
	// ハープーンガンのパラメータ
	float cooltime_ = 0.0f;               // 発射クールタイム
	float harpoonSpeed_ = 120.0f;         // 弾速
	float harpoonHomingStrength_ = 0.02f; // 追尾力
	float harpoonMaxDistance_ = 18.0f;    // 有効射程（メートル）
	float preTriggerR_ = 0.0f;            // 前フレームのRT（右トリガー）の入力値
};
