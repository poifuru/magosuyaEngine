#pragma once
#ifndef __HLSL_VERSION
#include <Windows.h>  // HLSL側では __HLSL_VERSION が定義されてないのでスキップできる
#endif
#include <memory>

class InputManager; // 前方宣言
extern std::unique_ptr<InputManager> g_inputManager;

class WindowsAPI {
public:	//静的メンバ関数
	static LRESULT CALLBACK WindowProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize ();

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