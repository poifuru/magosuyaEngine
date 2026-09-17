#pragma once
#include "FrameRateController.h"

class Time {
public:
	// ゲーム用デルタタイム取得関数(一時停止やスロー再生が出来る)
	static float GetDeltaTime() {
		return MyEngine::LowLevel::FrameRateController::GetInstance()->GetDeltaTime();
	}

	// 生のデルタタイム（ポーズ中もUIなどを動かしたい時に使う）
	static float GetUnscaledDeltaTime() {
		return MyEngine::LowLevel::FrameRateController::GetInstance()->GetUnscaledDeltaTime();
	}

	// ゲーム開始からの合計時間（秒）
	static float GetTime() {
		return MyEngine::LowLevel::FrameRateController::GetInstance()->GetTotalTime();
	}

	// タイムスケールの取得
	static float GetTimeScale() {
		return MyEngine::LowLevel::FrameRateController::GetInstance()->GetTimeScale();
	}

	// タイムスケールの変更
	static void SetTimeScale(float scale) {
		MyEngine::LowLevel::FrameRateController::GetInstance()->SetTimeScale(scale);
	}
};