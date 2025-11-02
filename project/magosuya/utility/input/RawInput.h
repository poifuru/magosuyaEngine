#pragma once 
#include <Windows.h>
#include <vector>  

// マウスボタンのインデックスを分かりやすくするために定義しとくでやんす！
enum MouseButton {
    LEFT = 0,
    RIGHT = 1,
    MIDDLE = 2,
};

class RawInput {  
public: // メンバ関数  
    void Initialize(HWND hwnd);
    void Update(LPARAM lParam);  

    //キーボードの入力関数
    bool Push (unsigned short key) const;
    bool Trigger (unsigned short key)const;
    bool Release (unsigned short key)const;

    //マウスの入力関数
    bool PushMouse (int button) const;
    bool TriggerMouse (int button) const;
    bool ReleaseMouse (int button) const;

    //デバッグ用
    void HandleMessageForDebug (LPARAM lParam);

    void EndFrame ();
    long GetMouseDeltaX() const { return mouseDeltaX_; }  
    long GetMouseDeltaY() const { return mouseDeltaY_; }  

private: // メンバ変数  
    //キーボード
    std::vector<bool> keys_ = std::vector<bool> (256, false);
    std::vector<bool> preKeys_ = std::vector<bool> (256, false);

    //マウス(左、右、ホイール対応で3ボタン)
    std::vector<bool> mouseButtons_ = std::vector<bool> (3, false);
    std::vector<bool> preMouseButtons_ = std::vector<bool> (3, false);

    long mouseDeltaX_ = 0;  
    long mouseDeltaY_ = 0;  

    //バッファ
    std::vector<BYTE> buffer_;
};
