#pragma once
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class GameObject;
class PlayerHealth;

class PlayerWeapon {
public:
	PlayerWeapon() = default;
	~PlayerWeapon();

	void Initialize();
	void Update(GameObject* gameObject, GameObject* reticleObject, PlayerHealth* playerHealth = nullptr);

	void Reload(PlayerHealth* playerHealth);

	void ImGui();
	void Serialize(json& j) const;
	void Deserialize(const json& j);

	// ゲッター・セッター
	float GetHarpoonMaxDistance() const { return harpoonMaxDistance_; }
	float GetFireInterval() const { return fireInterval_; }
	void SetFireInterval(float interval) { fireInterval_ = interval; }

	int GetAmmo() const { return ammo_; }
	int GetMaxAmmo() const { return maxAmmo_; }
	bool IsReloading() const { return isReloading_; }
	int GetReloadCost() const { return reloadCost_; }
	void SetMaxAmmo(int maxAmmo) { maxAmmo_ = maxAmmo; }
	void SetReloadCost(int cost) { reloadCost_ = cost; }

private:
	void Shoot(GameObject* gameObject, GameObject* reticleObject, PlayerHealth* playerHealth);
	void CreateUI(GameObject* gameObject);
	void UpdateUI();

private:
	// ハープーンガンのパラメータ
	float cooltime_ = 0.0f;               // 発射クールタイム
	float fireInterval_ = 0.15f;          // 連射間隔（秒）
	float harpoonSpeed_ = 120.0f;         // 弾速
	float harpoonHomingStrength_ = 0.02f; // 追尾力
	float harpoonMaxDistance_ = 80.0f;    // 有効射程（メートル）
	float preTriggerR_ = 0.0f;            // 前フレームのRT（右トリガー）の入力値

	// マガジン・リロードパラメータ
	int ammo_ = 15;                       // 現在残弾数
	int maxAmmo_ = 15;                    // 最大装填数
	int reloadCost_ = 10;                 // リロード消費電力
	float reloadDuration_ = 0.8f;         // リロード所要時間（秒）
	float reloadTimer_ = 0.0f;            // リロードタイマー
	bool isReloading_ = false;            // リロード中フラグ

	// 残弾UI用オブジェクト（外枠・背景・残弾バー・デジタル数字）
	GameObject* ammoBorderObj_ = nullptr;
	GameObject* ammoBgObj_ = nullptr;
	GameObject* ammoBarObj_ = nullptr;
	GameObject* ammoNumObj_ = nullptr;
	bool uiCreated_ = false;
};
