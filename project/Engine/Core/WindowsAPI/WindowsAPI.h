#pragma once

// 前方宣言
class InputManager; 

namespace MyEngine::LowLevel {
	class Engine;
}

class WindowsAPI {
public:
	// シングルトンインスタンスの取得
	static WindowsAPI* GetInstance();

	void Initialize(int32_t width, int32_t height);

	bool ProcessMessage();

	void Finalize();

	// フルスクリーン切り替え関数
	void SetFullscreen(bool fullscreen, bool borderless);

	bool IsFullscreen() const { return isFullscreen_; }
	bool IsBorderless() const { return isBorderless_; }

	// Engineの登録用（リサイズ連携のため）
	void RegisterEngine(MyEngine::LowLevel::Engine* engine) { engine_ = engine; }

	HWND GetHwnd() { return hwnd_; }
	int32_t& GetWindowWidth() { return windowWidth_; }
	int32_t& GetWindowHeight() { return windowHeight_; }
	float GetAspectRatio() const { return static_cast<float>(windowWidth_) / static_cast<float>(windowHeight_); }

private:
	WindowsAPI() = default;
	~WindowsAPI() = default;

	// コピー・移動禁止
	WindowsAPI(const WindowsAPI&) = delete;
	WindowsAPI& operator=(const WindowsAPI&) = delete;
	WindowsAPI(WindowsAPI&&) = delete;
	WindowsAPI& operator=(WindowsAPI&&) = delete;

private:	//staticメンバ関数
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

private:	//メンバ変数
	WNDCLASS windowClass_{};
	HWND hwnd_ = nullptr;
	int32_t windowWidth_ = 1280;
	int32_t windowHeight_ = 720;

	WINDOWPLACEMENT windowPlacement_{ sizeof(WINDOWPLACEMENT) };
	bool isFullscreen_ = false;
	bool isBorderless_ = false;

	MyEngine::LowLevel::Engine* engine_ = nullptr; // Engineインスタンスへのポインタ
};