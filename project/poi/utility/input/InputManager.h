#pragma once
#include "../../header/WinSupport.h"
#include <memory>
#include "RawInput.h"
#include "GamePad.h"

class InputManager {
public:	//メンバ関数
	void Initialize (HWND hwnd);
	void Update (LPARAM lparam);
	//preKeys更新用
	void EndFrame ();

	RawInput* GetRawInput () { return rawInput_.get(); }

private://メンバ変数
	std::unique_ptr<RawInput> rawInput_;
};