#pragma once
#include <Windows.h>
#include <Xinput.h>
#include <vector>
#include "struct.h"

enum Button {
	A,
	B,
	X,
	Y,
	L,
	R,
	CROSS_UP,
	CROSS_DOWN,
	CROSS_LEFT,
	CROSS_RIGHT,
	START,
	BACK,
	PUSHIN_LEFT,
	PUSHIN_RIGHT,
};

enum LR {
	Left,
	Right,
};

class GamePad {
public:
	void Initialize ();
	void Update ();

	//ボタン
	//指定したボタンを押し続けているか
	bool PushButton (Button button);
	//指定したボタンを押した瞬間
	bool TriggerButton (Button button);
	//指定したボタンを離した瞬間
	bool ReleaseButton (Button button);

	//スティックの傾きを0.0f ~ 1.0fの範囲で返す
	Vector2 GetStick (LR dir);

	//トリガーの押し込みを0.0f ~ 1.0fの範囲で返す
	float GetTrigger (LR dir);

	/// <summary>
	/// //左右のモーターの振動の強さを0.0f ~ 1.0fの範囲で設定する
	/// </summary>
	/// <param name="leftMotor">左モーター(低周波、強い振動)</param>
	/// <param name="rightMotor">右モーター(高周波、弱い振動)</param>
	/// <param name="rumbleTime">振動させる時間(秒)</param>
	void Vibration (float leftMotor, float rightMotor, float rumbleTime);

	bool IsConection ();

private:
	void SetRumbleTime (float time) { rumbleTimer_ = time; }
	void SetVibration (float leftMotor, float rightMotor) { leftMotor_ = leftMotor; rightMotor_ = rightMotor; }

private:
	//コントローラーの状態取得
	XINPUT_STATE state_ = {};
	//1フレーム前の状態
	XINPUT_STATE preState_ = {};
	//何番目のコントローラーか
	DWORD controllerIndex_ = 0;

	//ボタンの変数
	std::vector<bool> button_ = std::vector<bool> (14, false);
	//1フレーム前の状態
	std::vector<bool> preButton_ = std::vector<bool> (14, false);

	//スティックのアナログ値(-32768 ~ 32767)
	SHORT thumbLX_ = 0;
	SHORT thumbLY_ = 0;
	SHORT thumbRX_ = 0;
	SHORT thumbRY_ = 0;

	//トリガーのアナログ値(0 ~ 255)
	BYTE triggerL_ = 0;
	BYTE triggerR_ = 0;

	//振動する秒数
	float leftMotor_ = 0.0f;
	float rightMotor_ = 0.0f;
	float rumbleTimer_ = 0.0f;
};

