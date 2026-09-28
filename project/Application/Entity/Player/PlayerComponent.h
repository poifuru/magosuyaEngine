#pragma once
#include "Component.h"
#include <memory>
#include "PlayerMovement.h"
#include "PlayerWeapon.h"
#include "PlayerTurret.h"
#include "PlayerHealth.h"

// 前方宣言
class MainCameraComponent;

class PlayerComponent : public Component {
public:
	PlayerComponent();
	~PlayerComponent() override = default;

	// Componentのライフサイクル関数
	void Initialize() override;
	void Update() override;
	void ImGui() override;

	// セーブ・ロード用の関数
	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	// ゲッター
	const char* GetName() const override { return "PlayerComponent"; }

	// レティクル自動バインド用
	void ResolveReticle(const std::vector<std::unique_ptr<GameObject>>& gameObjects);

	// ダメージ処理・ステータス
	void TakeDamage(int damage);
	int GetHp() const;
	int GetMaxHp() const;
	bool IsDead() const;

	const Vector3& GetForward() const;

	// 水中フェーズ
	void TransitionToUnderwater();
	bool IsUnderwater() const;

	// サブシステムへのアクセサ
	PlayerMovement* GetMovement() const { return movement_.get(); }
	PlayerWeapon* GetWeapon() const { return weapon_.get(); }
	PlayerTurret* GetTurret() const { return turret_.get(); }
	PlayerHealth* GetHealth() const { return health_.get(); }

private:
	// サブシステム
	std::unique_ptr<PlayerMovement> movement_;
	std::unique_ptr<PlayerWeapon> weapon_;
	std::unique_ptr<PlayerTurret> turret_;
	std::unique_ptr<PlayerHealth> health_;

	// 外部参照用ポインタ
	GameObject* reticleObject_ = nullptr;
};