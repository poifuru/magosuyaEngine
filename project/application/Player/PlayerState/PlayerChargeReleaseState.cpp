#include "PlayerState.h"
#include "../Player.h"

void PlayerChargeReleaseState::Initialize() {
    if (!player_) return;

    releaseTimer_ = 0.0f;

    // チャージレベルに応じた攻撃アニメーション再生
    // if (chargeLevel_ == 2) { player_->PlayChargeAttackLevel2Animation(); }
    // else if (chargeLevel_ == 1) { player_->PlayChargeAttackLevel1Animation(); }
    // else { player_->PlayNormalAttackAnimation(); }

    // 攻撃モーション中のスタミナ回復をブロック
    //player_->BlockStaminaRecovery();

    // チャージレベルに応じて攻撃持続時間を調整しても良い
    // if (chargeLevel_ == 2) { maxReleaseDuration_ = 1.2f; }
}

void PlayerChargeReleaseState::Update() {
    const float deltaTime = 1.0f / 60.0f;
    releaseTimer_ += deltaTime;

    // 攻撃判定の計算と有効化
    // チャージレベル (chargeLevel_) に応じて、ヒットボックスの大きさやダメージを変更する
    // 例: Level 2 は範囲が広く、持続時間も長い

    // 攻撃の開始と終了タイミング (例: 0.1秒から 0.5秒まで判定を出す)
    if (releaseTimer_ >= 0.1f && releaseTimer_ <= maxReleaseDuration_ - 0.3f) {
        Vector3 attackOffset = {};
        float colliderRadius = 0.0f;

        if (chargeLevel_ >= 3) {
            // Level 3 の場合
            attackOffset.z = 0.0f;
            colliderRadius = 3.5f;
        }
        else if (chargeLevel_ == 2) {
            // Level 2 の場合
            attackOffset.z = 0.0f;
            colliderRadius = 2.0f;
        }
        else if(chargeLevel_ <= 1){
            // Level 1 以下の通常チャージの場合
            attackOffset.z = 0.0f;
            colliderRadius = 1.5f;
        }

        // 攻撃判定を有効化
        player_->SetAttackColliderRadius(colliderRadius);
        player_->EnableHitBox(true, attackOffset);
        player_->SetIsViewAttack(true);
    }
    else {
        // 攻撃判定が無効な時間
        player_->EnableHitBox(false, Vector3{});
        player_->SetIsViewAttack(false);
    }

    // ** 状態遷移ロジック (終了判定) **
    if (releaseTimer_ >= maxReleaseDuration_) {
        // 攻撃が終了したら、元の状態に戻る
        player_->ChangeState(new PlayerStopState());
        return;
    }

    if (!player_->IsOnGround()) {
        player_->ChangeState(new PlayerFallState());
        return;
    }
}

void PlayerChargeReleaseState::Exit() {
    if (!player_) return;

    // スタミナ回復ブロックの解除
    //player_->UnblockStaminaRecovery();

    // 攻撃判定を確実に無効化
    player_->EnableHitBox(false, player_->GetPosition());
    player_->SetIsViewAttack(false);
}