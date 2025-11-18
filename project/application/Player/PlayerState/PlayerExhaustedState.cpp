#include "PlayerState.h" // PlayerExhaustedStateの定義
#include "Player3D.h"    // Player3Dの機能を使うため

void PlayerExhaustedState::Initialize(){
    if (!player_) return;
    // 初期化時、タイマーをリセット
    exhaustedTimer_ = 0.0f;

    player_->BlockStaminaRecovery();
}

void PlayerExhaustedState::Update(){
    if (!player_) return;
    bool isMove = false;
    auto& move = player_->Move();
    if (InputManager::GetKey().PressKey(DIK_W))
    {
        move.z = 1.0f;
        isMove = true;
    }
    if (InputManager::GetKey().PressKey(DIK_S))
    {
        move.z = -1.0f;
        isMove = true;
    }
    if (InputManager::GetKey().PressKey(DIK_A))
    {
        move.x = -1.0f;
        isMove = true;
    }
    if (InputManager::GetKey().PressKey(DIK_D))
    {
        move.x = 1.0f;
        isMove = true;
    }

    const float deltaTime = 1.0f / 60.0f; // 例: デルタタイム

    // 1. タイマー更新
    exhaustedTimer_ += deltaTime;

    // 2. 移動の制限
    // スタミナ切れ中は動くとクソ遅い
    player_->SetSpeedMultiplier(0.2f);

    // 3. スタミナ回復ロジック

    // 遅延時間後に回復処理を開始
    if (exhaustedTimer_ >= STAMINA_REGEN_DELAY)
    {
        // Player3D側でスタミナ回復処理を呼び出す
        player_->UnblockStaminaRecovery();
    }

    // 4. 復帰判定 (スタミナが十分に回復したか？)
    

    if (player_->GetStaminaRate() * 100.0f >= EXHAUSTED_REGEN_THRESHOLD) {
        // 回復したら、通常の状態（待機状態など）に戻る
        player_->ChangeState(new PlayerStopState());
        return;
    }

    ImGuiManager::GetInstance()->Text("ExhaustedState");
}

void PlayerExhaustedState::Exit() {
    // 速度を戻す
    player_->SetSpeedMultiplier(1.0f);
}