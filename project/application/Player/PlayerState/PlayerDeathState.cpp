#include "PlayerState.h" 
#include "Player3D.h"    

void PlayerDeathState::Initialize()
{
	if (!player_) return;

	// 1. **タイマーのリセット**
	deathTimer_ = 0.0f;

	// 2. **移動・操作の停止**

	// 3. **当たり判定の完全無効化**
	// Player3D::TakeDamage ですでに無効化しているが、State側でも確認・徹底
	//player_->bodyCollider_->SetMyType(COL_None); 
}

void PlayerDeathState::Update()
{
	if (!player_) return;

	// デルタタイムを想定
	const float deltaTime = 1.0f / 60.0f;
	deathTimer_ += deltaTime;

	// 1. **移動の停止** (Updateでも念のため停止を継続)
	
	// 2. **演出タイマーのチェック**
	if (deathTimer_ >= MAX_DEATH_DURATION)
	{
		// 演出終了後、ゲームオーバー画面やリザルト画面に遷移する処理を呼び出す
		// 例: SceneManager::GetInstance()->ChangeScene(SceneType::GameOver); 

		// 死亡演出後の処理が完了したら、このStateのUpdateは基本的に何もしなくて良い
		// シーン遷移が完了すると、この Player3D インスタンスも破棄される想定
	}

	ImGuiManager::GetInstance()->Text("DeathState");
}

// Exit() は空のまま