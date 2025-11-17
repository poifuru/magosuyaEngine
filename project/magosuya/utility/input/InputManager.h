#pragma once
#include <Windows.h>
#include <memory>
#include "RawInput.h"
#include "GamePad.h"

class InputManager {
public:	//メンバ関数
	void Initialize (HWND hwnd);
	void Update (LPARAM lparam);
	//preKeys更新用
	void EndFrame ();

	RawInput* GetRawInput () { return rawInput_.get (); }
	GamePad* GetGamePad () { return gamePad_.get (); }

private://メンバ変数
	std::unique_ptr<RawInput> rawInput_;
	std::unique_ptr<GamePad> gamePad_;
};