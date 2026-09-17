#pragma once

namespace MyEngine::LowLevel {
	class FrameRateController {
	public:
		static FrameRateController* GetInstance () {
			//初めて呼び出されたときに一回だけ初期化
			static FrameRateController instance;
			return &instance;
		}
		FrameRateController();
		~FrameRateController() = default;

		/// <summary>
		/// フレームの開始時に呼び出して、経過時間を計算する
		/// </summary>
		void Update();

		// --- アクセッサ --- //
		float GetFrameRate() const { return frameRate_; }					// フレームレート
		float GetDeltaTime() const { return deltaTime_; }					// デルタタイム
		float GetUnscaledDeltaTime() const { return unscaledDeltaTime_; }	// 生のデルタタイム
		float GetTotalTime() const { return totalTime_; }					// 通算経過時間
		float GetTimeScale() const { return timeScale_; }					// タイムスケール
		void SetTimeScale(float scale) { timeScale_ = scale; }

	public:
		// コピー・移動禁止
		FrameRateController(const FrameRateController&) = delete;
		FrameRateController& operator=(const FrameRateController&) = delete;
		FrameRateController(FrameRateController&&) = delete;
		FrameRateController& operator=(FrameRateController&&) = delete;

	private:
		// 精密な時間計測のためのタイムスタンプ型
		std::chrono::steady_clock::time_point lastTime_;

		float frameRate_ = 0.0f; // 現在のFPS
		float deltaTime_ = 0.0f; // 前フレームからの経過時間（秒）
		float unscaledDeltaTime_ = 0.0f;  // 生のデルタタイム
		float totalTime_ = 0.0f;          // ゲーム開始からの合計時間
		float timeScale_ = 1.0f;          // タイムスケール（初期値 1.0）
	};
}