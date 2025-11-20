#include "CenterStomp.h"
#include "MathFunction.h"
#include "../../Boss.h"
#include <imgui.h>
#include <algorithm>

CenterStomp::CenterStomp(MagosuyaEngine* magosuya, Boss* boss) {
	magosuya_ = magosuya;
	boss_ = boss;

	// 攻撃範囲表示用のモデル作成
	model_ = std::make_unique<Model>(magosuya);
	magosuya_->LoadModelData("Resources/teapot", "teapot");
}

CenterStomp::~CenterStomp() {
}

void CenterStomp::Initialize() {
	model_->SetModelData("teapot");
	model_->SetTexture("teapot");
	model_->Initialize();

	// 初期位置リセット
	transform_ = { {2.0f,2.0f,2.0f},{0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };

	phase_ = StompPhase::None;
	isAliveWave_ = false;
}

void CenterStomp::StartAttack() {
	// 既に攻撃中なら無視
	if (phase_ != StompPhase::None) return;

	// フェーズを上昇へ設定
	phase_ = StompPhase::Rise;
	timer_ = 0;
	duration_ = 60; // 60フレーム（約1秒）かけて移動

	// ■ 線形補間の準備
	// 「今の場所」から「中央上空」までの移動ルートをここで確定させます
	startPos_ = boss_->GetPosition();     // スタート地点：今のボスの位置
	targetPos_ = kCenterPoint_;           // ゴール地点X,Z：中央(0,0)
	targetPos_.y = kHoverHeight_;         // ゴール地点Y：上空(10.0)
}

void CenterStomp::Update(Matrix4x4* m) {
	// 状態遷移マシンの実行
	switch (phase_) {
	case StompPhase::Rise:
		UpdateRise();
		isAliveWave_ = false;
		break;
	case StompPhase::Hover:
		UpdateHover();
		isAliveWave_ = false;
		break;
	case StompPhase::Fall:
		UpdateFall();
		isAliveWave_ = false;
		break;
	case StompPhase::Cooldown:
		UpdateCooldown();
		isAliveWave_ = true;
		break;
	case StompPhase::None:
		isAliveWave_ = false;
		break; // break漏れ修正
	default:
		break;
	}

	// ■ 修正点: 攻撃エフェクト（波）の位置を更新
	// もしエフェクトが常に(0,0,0)に出るべきならこのままで良いですが、
	// 念のためターゲット位置に合わせる処理を入れると確実です
	// transform_.translate = kCenterPoint_; 

	model_->SetTransform(transform_);
	model_->Update(m);
}

// 毎フレーム呼ばれる上昇更新処理
void CenterStomp::UpdateRise() {
	timer_++;

	// 進行度 t (0.0 ～ 1.0) を計算
	float t = static_cast<float>(timer_) / static_cast<float>(duration_);
	if (t > 1.0f) t = 1.0f;

	// ■ 線形補間 (Lerp) の実行
	// startPos_ から targetPos_ へ、t の割合だけ進んだ位置を計算
	// これにより瞬間移動ではなく、スムーズに移動します
	Vector3 currentPos = Lerp(startPos_, targetPos_, t);

	// 計算した位置をボスに反映
	boss_->SetPosition(currentPos);

	// 移動時間が終わったら次へ
	if (timer_ >= duration_) {
		// 念のため最終位置をきっちり合わせる（計算誤差防止）
		boss_->SetPosition(targetPos_);

		phase_ = StompPhase::Hover;
		timer_ = 0;
		duration_ = 90; // 待機時間へ
	}
}

void CenterStomp::UpdateHover() {
	timer_++;
	// 待機中は移動しないので座標更新不要

	// 待機完了
	if (timer_ >= duration_) {
		phase_ = StompPhase::Fall;
		timer_ = 0;
		duration_ = 20; // 急降下時間

		// ■ 修正点: 落下の準備
		// boss_->GetPosition()を使うと微細なズレを引き継ぐ可能性があるので
		// 論理的な座標を指定する方が安全です
		startPos_ = kCenterPoint_;
		startPos_.y = kHoverHeight_; // (0, 10, 0) から

		targetPos_ = kCenterPoint_;  // (0, 0, 0) へ
		targetPos_.y = 0.0f;         // 念のためYを0に明示
	}
}

void CenterStomp::UpdateFall() {
	timer_++;
	float t = static_cast<float>(timer_) / static_cast<float>(duration_);
	if (t > 1.0f) t = 1.0f;

	// 急降下 (EaseInQuad: t*t)
	Vector3 currentPos = Lerp(startPos_, targetPos_, t * t);

	boss_->SetPosition(currentPos);

	// 地面に着弾
	if (timer_ >= duration_) {
		// ■ 修正点: 着弾時は必ず目標座標にスナップ
		boss_->SetPosition(targetPos_);

		// ★ここでエフェクトの位置を更新（必要であれば）
		transform_.translate = targetPos_;

		// ダメージ判定などをここに記述
		// CheckCollision(); 

		phase_ = StompPhase::Cooldown;
		timer_ = 0;
		duration_ = 30; // 硬直時間
	}
}

void CenterStomp::UpdateCooldown() {
	timer_++;
	if (timer_ >= duration_) {
		// すべて終了、通常状態へ戻る
		phase_ = StompPhase::None;

		// ■ 重要: ボスが通常行動（UpdateMove）に戻ったとき、
		// (0,0,0)からスムーズに動けるようにボス側の内部状態も整合性が取れているはずです。
		// Boss.cpp の UpdateMove は現在の座標から加算しているので問題ありません。
	}
}

void CenterStomp::Draw() {
	if (isAliveWave_) {
		model_->Draw();
	}
}

void CenterStomp::ImGuiControl() {
#ifdef _DEBUG
	ImGui::Begin("CenterStomp Attack");

	// 攻撃テスト用ボタン
	if (ImGui::Button("Start Stomp")) {
		StartAttack();
	}

	// 現在の状態表示
	const char* stateStr = "None";
	switch (phase_) {
	case StompPhase::Rise: stateStr = "Rise"; break;
	case StompPhase::Hover: stateStr = "Hover"; break;
	case StompPhase::Fall: stateStr = "Fall"; break;
	case StompPhase::Cooldown: stateStr = "Cooldown"; break;
	}
	ImGui::Text("State: %s", stateStr);
	ImGui::Text("Timer: %d / %d", timer_, duration_);

	ImGui::End();
#endif
}