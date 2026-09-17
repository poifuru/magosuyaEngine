#include "PCH.h"
#include "FrameRateController.h"

MyEngine::LowLevel::FrameRateController::FrameRateController() {
	// 初期化時点のタイムスタンプを記録しておく
	lastTime_ = std::chrono::steady_clock::now();
}

void MyEngine::LowLevel::FrameRateController::Update() {
	// 現在の時間を取得
	std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();

	// 前回からの経過時間をマイクロ秒単位で計算
	auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastTime_);

	// 次のフレームのために現在の時間を保存
	lastTime_ = currentTime;

	// マイクロ秒から「秒」に変換してデルタタイムにする (1秒 = 1,000,000マイクロ秒)
	float rawDeltaTime = static_cast<float>(elapsedTime.count()) / 1000000.0f;
	if (rawDeltaTime > 0.1f) rawDeltaTime = 0.1f; // キャップ処理

	// 生のデルタタイムを保存
	unscaledDeltaTime_ = rawDeltaTime;

	// timeScale を掛け算してゲーム用デルタタイムを作る
	deltaTime_ = unscaledDeltaTime_ * timeScale_;

	// 合計時間を加算
	totalTime_ += deltaTime_;

	// 現在のFPSの計算（1秒 / 1フレームの時間）
	if (unscaledDeltaTime_  > 0.0f) {
		frameRate_ = 1.0f / unscaledDeltaTime_ ;
	}
}