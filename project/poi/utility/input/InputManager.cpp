#include "InputManager.h"

void InputManager::Initialize (HWND hwnd) {
	rawInput_ = std::make_unique<RawInput> ();
	rawInput_->Initialize (hwnd);
}

void InputManager::Update (LPARAM lparam) {
	rawInput_->Update (lparam);
}

void InputManager::EndFrame () {
	rawInput_->EndFrame ();
}