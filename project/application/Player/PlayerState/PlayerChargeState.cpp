#include "PlayerState.h"
#include "../Player.h"

void PlayerChargeState::Initialize() {
    if (!player_) return;

    // チャージしているときはスタミナを回復させるかどうか迷う


    // チャージタイマーリセット
    chargeTimer_ = 0.0f;
    currentChargeLevel_ = 0;

    // チャージ開始のアニメーションやエフェクト再生
    // player_->PlayChargeStartAnimation(); 
}

void PlayerChargeState::Update() {
    const float deltaTime = 1.0f / 60.0f;

    // 1. 入力チェック: ボタンが離されたか？
    if (player_->engine_->GetRawInput()->Push('J') == false) {
        // ボタンが離されたら、チャージ解放状態へ遷移
        player_->ChangeState(new PlayerChargeReleaseState(currentChargeLevel_));
        return;
    }

    // 2. タイマー更新
    chargeTimer_ += deltaTime;

    // 3. チャージレベルの判定
    // 例: 0.5秒で Level 1, 1.5秒で Level 2 (MAX)
    if (chargeTimer_ < 0.5f) {
        currentChargeLevel_ = 1; // Level 1 チャージ
    }
    else if (chargeTimer_ < 1.5f) {
        currentChargeLevel_ = 2; // Level 2 チャージ
        // エフェクトの変更
    }
    else {
        currentChargeLevel_ = 3; // Level 3 チャージ (MAX)
        // 最大チャージ時のエフェクト
    }

    // 4. 最大時間到達判定(これは正直いらないかな)
    //if (chargeTimer_ >= MAX_CHARGE_TIME) {
    //    // 最大チャージ時間に達したら、自動的にチャージ解放状態へ遷移
    //    player_->ChangeState(new PlayerChargeReleaseState(currentChargeLevel_));
    //    return;
    //}

    if (!player_->IsOnGround()) {
        // 空中判定（崖から落ちた場合など）
        player_->ChangeState(new PlayerFallState());
        return;
    }

    // 回避でキャンセル出来るようにしてもいい
    /*if (もし回避行動をしたら) {
        回避のStateに遷移させる
        return;
    }*/
}

void PlayerChargeState::Exit() {
    if (!player_) return;

    // スタミナ回復ブロックの解除
    //player_->UnblockStaminaRecovery();

    // チャージエフェクトを停止
    // player_->StopChargeEffect();
}