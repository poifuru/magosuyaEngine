#include "PCH.h"
#include "WindowsAPI.h"
#include "InputManager.h"
#include "RawInput.h"
#include "Engine.h"

WindowsAPI* WindowsAPI::GetInstance() {
	static WindowsAPI instance;
	return &instance;
}

void WindowsAPI::Initialize(int32_t width, int32_t height) {
	windowWidth_ = width;
	windowHeight_ = height;

	//ウィンドウプロシージャ
	windowClass_.lpfnWndProc = WindowProc;
	//ウィンドウクラス名
	windowClass_.lpszClassName = L"MagosuyaEngine";
	//インスタンスハンドル
	windowClass_.hInstance = GetModuleHandle(nullptr);
	//カーソル
	windowClass_.hCursor = LoadCursor(nullptr, IDC_ARROW);

	//ウィンドウクラスを登録する
	RegisterClass(&windowClass_);

	//ウィンドウサイズを表す構造体にクライアント領域を入れる
	RECT wrc = { 0, 0, windowWidth_, windowHeight_ };

	//クライアント領域を元に実際のサイズにwrcを変更してもらう
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//ウィンドウを生成
	hwnd_ = CreateWindow(
		windowClass_.lpszClassName,	//利用するクラス名
		L"DirectX_Game",			//タイトルバーの文字
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
	assert(hwnd_ != nullptr);

	// ウィンドウに this ポインタを保存
	SetWindowLongPtr(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

	//ウィンドウを表示
	ShowWindow(hwnd_, SW_SHOW);
}

// Win32の標準のWndProcHandler呼び出し用前方宣言
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WindowsAPI::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}
	
	switch (msg) {
	case WM_SIZE:
		if (wparam != SIZE_MINIMIZED) {
			// 保存しておいたポインタを取り出す
			WindowsAPI* self = reinterpret_cast<WindowsAPI*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
			if (self) {
				int32_t width = LOWORD(lparam);
				int32_t height = HIWORD(lparam);
				self->windowWidth_ = width;
				self->windowHeight_ = height;
				// Engineが登録されていればリサイズを通知
				if (self->engine_) {
					self->engine_->OnResize(width, height);
				}
			}
		}
		break;

	case WM_INPUT:
		InputManager::GetInstance()->GetRawInput()->HandleInputMessage(lparam);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

bool WindowsAPI::ProcessMessage() {
	MSG msg{};
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_QUIT) {
			return true;
		}
	}
	return false;
}

void WindowsAPI::Finalize() {
	if (hwnd_) {
		DestroyWindow(hwnd_);
		hwnd_ = nullptr;
	}
}

void WindowsAPI::SetFullscreen(bool fullscreen, bool borderless) {
	if (isFullscreen_ == fullscreen && isBorderless_ == borderless) return;

	if (fullscreen) {
		if (borderless) {
			// ボーダレスウィンドウの場合
			// ウィンドウの現在の状態を保存
			GetWindowPlacement(hwnd_, &windowPlacement_);

			// モニター全体のサイズを取得
			HMONITOR monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
			MONITORINFO monitorInfo{};
			monitorInfo.cbSize = sizeof(MONITORINFO);
			GetMonitorInfo(monitor, &monitorInfo);

			// スタイルを枠無しのポップアップに変更
			LONG style = GetWindowLong(hwnd_, GWL_STYLE);
			style &= ~WS_OVERLAPPEDWINDOW; // 枠やタイトルバーに関するフラグをすべて消去
			style |= WS_POPUP | WS_VISIBLE;
			SetWindowLong(hwnd_, GWL_STYLE, style);

			LONG exStyle = GetWindowLong(hwnd_, GWL_EXSTYLE);
			exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE);
			SetWindowLong(hwnd_, GWL_EXSTYLE, exStyle);

			// ウィンドウサイズを画面全体に合わせる（ここで自動的に WM_SIZE が走る）
			SetWindowPos(hwnd_, HWND_TOP,
						 monitorInfo.rcMonitor.left,
						 monitorInfo.rcMonitor.top,
						 monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
						 monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
						 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
			windowWidth_ = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
			windowHeight_ = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
		}
	} 
	else {
		// ウィンドウモードに戻す場合
		if (isBorderless_) {
			// 通常のウィンドウスタイルに戻す
			LONG style = GetWindowLong(hwnd_, GWL_STYLE);
			style &= ~WS_POPUP;
			style |= WS_OVERLAPPEDWINDOW | WS_VISIBLE;
			SetWindowLong(hwnd_, GWL_STYLE, style);

			// 拡張スタイルをデフォルト（0）に戻す
			SetWindowLong(hwnd_, GWL_EXSTYLE, 0);

			// ウィンドウ位置とサイズを元に戻す
			SetWindowPlacement(hwnd_, &windowPlacement_);
			SetWindowPos(hwnd_, nullptr, 0, 0, 0, 0,
						 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}

		// クライアント領域のサイズを再取得
		RECT clientRect;
		GetClientRect(hwnd_, &clientRect);
		windowWidth_ = clientRect.right - clientRect.left;
		windowHeight_ = clientRect.bottom - clientRect.top;
	}

	isFullscreen_ = fullscreen;
	isBorderless_ = borderless;
}