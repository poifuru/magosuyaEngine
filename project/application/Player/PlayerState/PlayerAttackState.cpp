#include "PlayerState.h"
#include "../Player.h"
#include "MathFunction.h"

void PlayerAttackState::Initialize(){
    if (!player_) return;

    // スタミナ消費と回復ブロック
    // 攻撃にもスタミナコストがある場合や、回復を止める場合
    player_->BlockStaminaRecovery();

    // タイマーリセット
    attackTimer_ = 0.0f;
    canCombo_ = false;

    // チャージ判定の初期化
    isAttackHeld_ = true;
}

void PlayerAttackState::Update(){
    const float deltaTime = 1.0f / 60.0f;
    attackTimer_ += deltaTime;

    // チャージに移行できる時間
    const float CHARGE_WINDOW_DURATION = 0.15f;

    if (attackTimer_ < CHARGE_WINDOW_DURATION) {
        if (player_->engine_->GetRawInput()->Push('J') == false) {
            isAttackHeld_ = false;
        }
    }

    // 攻撃判定の位置を計算
    Vector3 playerPos = player_->GetPosition();
    Vector3 attackOffset = {};// プレイヤーからの相対位置
    float colliderRadius = 0.0f;
    Vector3 hitBoxWorldPos{};
    // ここで位置計算
    if (attackTimer_ >= CHARGE_WINDOW_DURATION && attackTimer_ <= 0.4f) {

        if (isAttackHeld_ == true) {
            // チャージ攻撃に遷移
            player_->ChangeState(new PlayerChargeState());
            return;
        }

        // 0.0f ~ 1.0fに正規化
        float slashTime = (attackTimer_ - 0.1f) / 0.3f;

        // 刀の軌道を計算
        /*if (slashTime < 0.5f) {
            attackOffset.y = 1.8f - (slashTime * 0.5f);
            attackOffset.z = 0.8f;
            colliderRadius = 0.6f;
        }
        else {
            attackOffset.y = 1.5f - (slashTime * 1.0f);
            attackOffset.z = 1.2f;
            colliderRadius = 0.8f;
        }*/
        attackOffset.y = 0.0f;
        attackOffset.z = 1.2f;
        colliderRadius = 0.8f;
        player_->SetAttackColliderRadius(colliderRadius);
        hitBoxWorldPos = attackOffset/*+ (Quaternion::RotateVector(attackOffset,player_->GetPlayerQuaternion()) * 2.0f)*/;
        player_->EnableHitBox(true, hitBoxWorldPos);
        player_->SetIsViewAttack(true);
    }
    else {
        //hitBoxWorldPos = playerPos + (attackOffset * player_->GetForwardVector());
        player_->EnableHitBox(false, hitBoxWorldPos);
        player_->SetIsViewAttack(false);
    }

    // コンボ受付期間の管理
    //if (attackTimer_ >= comboWindowStart_ && attackTimer_ <= comboWindowEnd_)
    //{
    //    canCombo_ = true;
    //    // 💡 この期間に次の攻撃入力があったかをチェックする
    //    if (/*アタックの攻撃ボタンの判定*/)
    //    {
    //        // 次の攻撃（コンボ）状態へ遷移
    //        player_->ChangeState(new PlayerComboAttackState());
    //        return;
    //    }
    //}
    //else
    //{
    //    canCombo_ = false;
    //}

    // 状態遷移ロジック (終了判定)
    if (attackTimer_ >= maxAttackDuration_){
        // 攻撃が終了したら、元の状態に戻る
        player_->ChangeState(new PlayerStopState());
        return;
    }

    
    if (!player_->IsOnGround()){
        // 空中判定（崖から落ちた場合など）
        player_->ChangeState(new PlayerFallState());
        return;
    }

    //ImGuiManager::GetInstance()->Text("AttackState");
}

void PlayerAttackState::Exit()
{
    if (!player_) return;

    // スタミナ回復ブロックの解除
    player_->UnblockStaminaRecovery();

    player_->SetIsViewAttack(false);

    // 攻撃判定を確実に無効化
    player_->EnableHitBox(false,player_->GetPosition());

    // 他に何かあれば

}