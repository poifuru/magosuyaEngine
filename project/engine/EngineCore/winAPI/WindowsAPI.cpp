#include "WindowsAPI.h"
#include "../../utility/function.h"
#include "../../Input/InputManager.h"

LRESULT WindowsAPI::WindowProc (HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	ImGui_ImplWin32_WndProcHandler (hwnd, msg, wparam, lparam);

	//メッセージに応じてゲーム固有の処理を行う
	switch (msg) {
		//入力を検知した
	case WM_INPUT:
		if (g_inputManager) {
			g_inputManager->Update (lparam);
		}
		break;

		//ウィンドウが破棄された
	case WM_DESTROY:
		//OSに対して、アプリの終了を伝える
		PostQuitMessage (0);
		return 0;
	}

	//標準のメッセージ処理を行う
	return DefWindowProc (hwnd, msg, wparam, lparam);
}

void WindowsAPI::Initalize () {
	//ウィンドウプロシージャ
	windowClass_.lpfnWndProc = WindowProc;
	//ウィンドウクラス名
	windowClass_.lpszClassName = L"LE2B_22_マスヤ_ゴウ";
	//インスタンスハンドル
	windowClass_.hInstance = GetModuleHandle (nullptr);
	//カーソル
	windowClass_.hCursor = LoadCursor (nullptr, IDC_ARROW);

	//ウィンドウクラスを登録する
	RegisterClass (&windowClass_);

	//ウィンドウサイズを表す構造体にクライアント領域を入れる
	RECT wrc = { 0, 0, kClientWidth, kClientHeight };

	//クライアント領域を元に実際のサイズにwrcを変更してもらう
	AdjustWindowRect (&wrc, WS_OVERLAPPEDWINDOW, false);

	//ウィンドウを生成
	hwnd_ = CreateWindow (
		windowClass_.lpszClassName,	//利用するクラス名
		L"CG2",						//タイトルバーの文字
		WS_OVERLAPPEDWINDOW,		//よく見るウィンドウスタイル
		CW_USEDEFAULT,				//表示x座標(Windowsに任せる)
		CW_USEDEFAULT,				//表示y座標(WindowsOSに任せる)
		wrc.right - wrc.left,		//ウィンドウ横幅
		wrc.bottom - wrc.top,		//ウィンドウ縦幅
		nullptr,					//親ウィンドウハンドル
		nullptr,					//メニューハンドル
		windowClass_.hInstance,		//インスタンスハンドル
		nullptr						//オプション
	);

	//ウィンドウを表示
	ShowWindow (hwnd_, SW_SHOW);
}

bool WindowsAPI::ProcessMessage () {
	MSG msg{};

	//Windowにメッセージが来てたら最優先で処理させる
	if (PeekMessage (&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}

	//終了するかの判断をここでしてboolで返す
	if (msg.message == WM_QUIT) {
		return true;
	}

	return false;
}

void WindowsAPI::Finalize () {
	DestroyWindow (hwnd_);
}