#pragma once
#include "../../../header/WinSupport.h"
#include "../../Input/InputManager.h"
#include <memory>

class WindowsAPI {
public:	//静的メンバ関数
	static LRESULT CALLBACK WindowProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initalize ();

	bool ProcessMessage ();

	void Finalize ();

	//アクセッサ
	HWND GetHwnd () { return hwnd_; }

public:	//定数
	//クライアント領域のサイズ
	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;

private:
	//ウィンドウ
	WNDCLASS windowClass_{};
	HWND hwnd_{};
};