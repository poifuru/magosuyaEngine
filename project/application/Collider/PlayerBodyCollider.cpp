#include "PlayerBodyCollider.h"
#include "../application/Player/Player.h"

PlayerBodyCollider::PlayerBodyCollider(Player* player) : player_(player)
{
	// 自身の属性: プレイヤー本体
	SetMyType(COL_Player);

	// 衝突対象: 敵の攻撃
	SetYourType(COL_Enemy_Attack);

	// デフォルトの当たり判定半径を設定
	SetRadius(1.0f);
}

void PlayerBodyCollider::OnCollision(Collider* other)
{
	if (!player_) return;

	// 1. **衝突相手が敵の攻撃であるかチェック**
	// 衝突相手が COL_Enemy_Attack 属性を持っているか確認
	if (other->GetMyType() & COL_Enemy_Attack)
	{
		// 既に死亡状態や無敵時間中の場合はダメージ処理を行わない
		// if (player_->IsDead() || player_->IsInvulnerable()) { return; }

		// 2. **ダメージ値を決定**
		// 💡 今回はシンプルに固定値で処理
		float damage = BASE_DAMAGE_TAKEN;

		// 3. **Player3D 本体にダメージを通知**
		// Player3D に TakeDamage(float) メソッドが必要です
		player_->TakeDamage(damage);
	}
}

const Vector3 PlayerBodyCollider::GetWorldPosition()
{
	if (!player_) return { 0.0f, 0.0f, 0.0f };
	// Player3D::GetPosition() はプレイヤーのワールド座標を返す想定
	return player_->GetPosition();
}