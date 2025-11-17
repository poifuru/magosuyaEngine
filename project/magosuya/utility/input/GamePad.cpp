#include "GamePad.h"
#pragma comment(lib, "Xinput.lib")
#include <imgui.h>

void GamePad::Initialize () {
	state_ = {};
	preState_ = {};
	controllerIndex_ = 0;

	button_ = std::vector<bool> (14, false);
	preButton_ = std::vector<bool> (14, false);

	//スティックのアナログ値(-32768 ~ 32767)
	thumbLX_ = 0;
	thumbLY_ = 0;
	thumbRX_ = 0;
	thumbRY_ = 0;

	//トリガーのアナログ値(0 ~ 255)
	triggerL_ = 0;
	triggerR_ = 0;

	//振動周り
	leftMotor_ = 0.0f;
	rightMotor_ = 0.0f;
	rumbleTimer_ = 0.0f;
}

void GamePad::Update () {
	// 1フレーム前の状態として保存
	preState_ = state_;
	preButton_ = button_;

	// 新しく状態をとるためにリセット
	button_.assign (14, false);

	// コントローラーが接続されていれば
	if (XInputGetState (controllerIndex_, &state_) == ERROR_SUCCESS) {
		//===更新処理===//
		// スティックとトリガー
		thumbLX_ = state_.Gamepad.sThumbLX;
		thumbLY_ = state_.Gamepad.sThumbLY;
		thumbRX_ = state_.Gamepad.sThumbRX;
		thumbRY_ = state_.Gamepad.sThumbRY;

		triggerL_ = state_.Gamepad.bLeftTrigger;
		triggerR_ = state_.Gamepad.bRightTrigger;

		// Aボタン
		if (state_.Gamepad.wButtons & XINPUT_GAMEPAD_A) {
			button_[Button::A] = true;
		}
		// Bボタン
		if (state_.Gamepad.wButtons & XINPUT_GAMEPAD_B) {
			button_[Button::B] = true;
		}
		// Xボタン
		if (state_.Gamepad.wButtons & XINPUT_GAMEPAD_X) {
			button_[Button::X] = true;
		}
		// Yボタン
		if (state_.Gamepad.wButtons & XINPUT_GAMEPAD_Y) {
			button_[Button::Y] = true;
		}
		// Lボタン
		if (state_.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) {
			button_[Button::L] = true;
		}
		// Rボタン
		if (state_.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) {
			button_[Button::R] = true;
		}
		// 十字_上
		if (state_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) {
			button_[Button::CROSS_UP] = true;
		}
		// 十字_下
		if (state_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) {
			button_[Button::CROSS_DOWN] = true;
		}
		// 十字_左
		if (state_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) {
			button_[Button::CROSS_LEFT] = true;
		}
		// 十字_右
		if (state_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) {
			button_[Button::CROSS_RIGHT] = true;
		}
		// スタート
		if (state_.Gamepad.wButtons & XINPUT_GAMEPAD_START) {
			button_[Button::START] = true;
		}
		// バック
		if (state_.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) {
			button_[Button::BACK] = true;
		}
		// 左スティック押し込み
		if (state_.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) {
			button_[Button::PUSHIN_LEFT] = true;
		}
		// 右スティック押し込み
		if (state_.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) {
			button_[Button::PUSHIN_RIGHT] = true;
		}

		//振動タイマーの処理
		if (rumbleTimer_ >= 0.0f) {
			rumbleTimer_ -= 1.0f / 60.0f;
		}
		else {
			SetVibration (0.0f, 0.0f);
			Vibration (leftMotor_, rightMotor_, rumbleTimer_);
		}
	}
	else {
		// 接続に失敗してるなら状態リセット
		state_ = {};
	}
}

bool GamePad::PushButton (Button button) {
	return button_[button];
}

bool GamePad::TriggerButton (Button button) {
	return !preButton_[button] && button_[button];
}

bool GamePad::ReleaseButton (Button button) {
	return preButton_[button] && !button_[button];
}

Vector2 GamePad::GetStick (LR dir) {
	// 戻り値
	Vector2 result = {};

	// スティックの値をもらう
	SHORT valueX = 0;
	SHORT valueY = 0;

	switch (dir) {
	case LR::Left:
		valueX = thumbLX_;
		valueY = thumbLY_;
		break;

	case LR::Right:
		valueX = thumbRX_;
		valueY = thumbRY_;
		break;
	}

	// デッドゾーンを超えているかチェック
	constexpr SHORT DEADZONE = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;

	// XとYそれぞれの軸でデッドゾーンを考慮
	if (valueX > DEADZONE || valueX < -DEADZONE) {
		result.x = static_cast<float> (valueX) / 32767.0f;
	}
	else {
		result.x = 0.0f;
	}

	if (valueY > DEADZONE || valueY < -DEADZONE) {
		result.y = static_cast<float> (valueY) / 32767.0f;
	}
	else {
		result.y = 0.0f;
	}

	return result;
}

float GamePad::GetTrigger (LR dir) {
	float result = 0.0f;

	BYTE value = 0;

	switch (dir) {
	case LR::Left:
		value = triggerL_;
		break;

	case LR::Right:
		value = triggerR_;
		break;
	}

	// デッドゾーンを超えているかチェック
	constexpr BYTE TRIGGER_DEADZONE = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;

	if (value > TRIGGER_DEADZONE) {
		result = static_cast<float>(value) / 255.0f;
	}
	else {
		result = 0.0f;
	}

	return result;
}

void GamePad::Vibration (float leftMotor, float rightMotor, float rumbleTime) {
	// コントローラーが接続されているかチェック
	if (!IsConection ()) {
		return;
	}

	rumbleTimer_ = rumbleTime;

	// XInputの振動構造体を用意する
	XINPUT_VIBRATION vibration;
	ZeroMemory (&vibration, sizeof (XINPUT_VIBRATION));

	// 0.0f〜1.0f の値を 0〜65535 の WORD 値に変換する
	// Clamp (0.0f〜1.0f の間に収める) して、65535 を掛ける

	// 左モーター (L Motor)
	float clampedLeft = std::fmax (0.0f, std::fmin (1.0f, leftMotor));
	vibration.wLeftMotorSpeed = static_cast<WORD>(clampedLeft * 65535.0f);

	// 右モーター (R Motor)
	float clampedRight = std::fmax (0.0f, std::fmin (1.0f, rightMotor));
	vibration.wRightMotorSpeed = static_cast<WORD>(clampedRight * 65535.0f);

	// 実際にコントローラーに振動を送信する
	XInputSetState (controllerIndex_, &vibration);
}

bool GamePad::IsConection () {
	XINPUT_STATE tmp{};
	return XInputGetState (controllerIndex_, &tmp) == ERROR_SUCCESS;
}
