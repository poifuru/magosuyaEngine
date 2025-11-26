#pragma once
#include <Windows.h>
#include <memory>
#include "RawInput.h"
#include "GamePad.h"

class InputManager {
public:	//メンバ関数
	static InputManager* GetInstance () {
		//初めて呼び出されたときに一回だけ初期化
		static InputManager instance;
		return &instance;
	}

	void Initialize (HWND hwnd);
	void Update (LPARAM lparam);
	//preKeys更新用
	void EndFrame ();

	RawInput* GetRawInput () { return rawInput_.get(); }

private:
	//コンストラクタを禁止
	InputManager () = default;
	// コピーコンストラクタと代入演算子を禁止
	InputManager (const InputManager&) = delete;
	InputManager& operator=(const InputManager&) = delete;
	InputManager (InputManager&&) = delete;
	InputManager& operator=(InputManager&&) = delete;

private://メンバ変数
	std::unique_ptr<RawInput> rawInput_;
};