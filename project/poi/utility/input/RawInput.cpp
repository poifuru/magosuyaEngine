#include "RawInput.h"
#include "../utility/function.h"
#include <hidusage.h>

void RawInput::Initialize (HWND hwnd) {
    OutputDebugStringA ("RawInput 登録\n");
    RAWINPUTDEVICE rid[2];

    // キーボード
    rid[0].usUsagePage = 0x01; // 汎用デスクトップコントロール
    rid[0].usUsage = 0x06;     // キーボード
    rid[0].dwFlags = RIDEV_INPUTSINK; // フォーカス外でも入力取得
    rid[0].hwndTarget = hwnd;

    // マウス
    rid[1].usUsagePage = 0x01;
    rid[1].usUsage = 0x02;     // マウス
    rid[1].dwFlags = RIDEV_INPUTSINK;
    rid[1].hwndTarget = hwnd;

    if (!RegisterRawInputDevices (rid, 2, sizeof (rid[0]))) {
        MessageBoxA (hwnd, "RawInput 登録失敗", "Error", MB_OK);
    }
    else {
        OutputDebugStringA ("RawInput 登録成功\n");
    }
}

void RawInput::Update (LPARAM lParam) {
    UINT size = 0;
    GetRawInputData ((HRAWINPUT)lParam, RID_INPUT, nullptr, &size, sizeof (RAWINPUTHEADER));
    if (buffer_.size () < size)
        buffer_.resize (size);

    if (GetRawInputData ((HRAWINPUT)lParam, RID_INPUT, buffer_.data (), &size, sizeof (RAWINPUTHEADER)) != size)
        return;

    RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer_.data ());

    // キーボード入力
    if (raw->header.dwType == RIM_TYPEKEYBOARD) {
        const RAWKEYBOARD& kb = raw->data.keyboard;
        USHORT key = kb.VKey;
        bool down = !(kb.Flags & RI_KEY_BREAK);
        if (key < 256) keys_[key] = down;
    }

    // マウス入力
    if (raw->header.dwType == RIM_TYPEMOUSE) {
        const RAWMOUSE& ms = raw->data.mouse;
        mouseDeltaX_ += ms.lLastX;
        mouseDeltaY_ += ms.lLastY;

        // マウスボタンの状態を更新
        USHORT buttonFlags = ms.usButtonFlags;

        // 左ボタン
        if (buttonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) mouseButtons_[LEFT] = true;
        if (buttonFlags & RI_MOUSE_LEFT_BUTTON_UP) mouseButtons_[LEFT] = false;

        // 右ボタン
        if (buttonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) mouseButtons_[RIGHT] = true;
        if (buttonFlags & RI_MOUSE_RIGHT_BUTTON_UP) mouseButtons_[RIGHT] = false;

        // 中ボタン
        if (buttonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) mouseButtons_[MIDDLE] = true;
        if (buttonFlags & RI_MOUSE_MIDDLE_BUTTON_UP) mouseButtons_[MIDDLE] = false;
    }
}

bool RawInput::Push (unsigned short key) const {
    return keys_[key];
}

bool RawInput::Trigger (unsigned short key) const {
    return (keys_[key] && !preKeys_[key]);
}

bool RawInput::Release (unsigned short key) const {
    return (!keys_[key] && preKeys_[key]);
}

bool RawInput::PushMouse (int button) const {
    if (button >= 0 && button < (int)mouseButtons_.size ()) {
        return mouseButtons_[button];
    }

    return false;
}

bool RawInput::TriggerMouse (int button) const {
    if (button >= 0 && button < (int)mouseButtons_.size ()) {
        return (mouseButtons_[button] && !preMouseButtons_[button]);
    }

    return false;
}

bool RawInput::ReleaseMouse (int button) const {
    if (button >= 0 && button < (int)mouseButtons_.size ()) {
        return (!mouseButtons_[button] && preMouseButtons_[button]);
    }

    return false;
}

void RawInput::HandleMessageForDebug (LPARAM lParam) {
    UINT size = 0;
    UINT ret = GetRawInputData ((HRAWINPUT)lParam, RID_INPUT, nullptr, &size, sizeof (RAWINPUTHEADER));
    char out[256];
    sprintf_s (out, "GetRawInputData(sizeQuery) returned %u size=%u\n", ret, size);
    OutputDebugStringA (out);

    if (size == 0) return;
    if (buffer_.size () < size) buffer_.resize (size);

    UINT bytes = GetRawInputData ((HRAWINPUT)lParam, RID_INPUT, buffer_.data (), &size, sizeof (RAWINPUTHEADER));
    sprintf_s (out, "GetRawInputData(data) returned %u bytes=%u\n", bytes, size);
    OutputDebugStringA (out);

    RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer_.data ());
    if (raw->header.dwType == RIM_TYPEMOUSE) {
        const RAWMOUSE& ms = raw->data.mouse;
        sprintf_s (out, "MOUSE: lX=%ld lY=%ld flags=%04x usFlags=%04x\n",
                   ms.lLastX, ms.lLastY, ms.usButtonFlags, ms.usFlags);
        OutputDebugStringA (out);
    }
    else if (raw->header.dwType == RIM_TYPEKEYBOARD) {
        const RAWKEYBOARD& kb = raw->data.keyboard;
        sprintf_s (out, "KEY: VKey=%u Flags=%04x\n", kb.VKey, kb.Flags);
        OutputDebugStringA (out);
    }
}

void RawInput::EndFrame () {
    //preKeysの状態を更新
    preKeys_ = keys_;

    // preMouseButtonsの状態を更新するでやんす！
    preMouseButtons_ = mouseButtons_;

    mouseDeltaX_ = 0;
    mouseDeltaY_ = 0;
}