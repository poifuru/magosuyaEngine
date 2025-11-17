#include "InputManager.h"

void InputManager::Initialize (HWND hwnd) {
	rawInput_ = std::make_unique<RawInput> ();
	rawInput_->Initialize (hwnd);
	gamePad_ = std::make_unique<GamePad> ();
	gamePad_->Initialize ();
}

void InputManager::Update (LPARAM lparam) {
	rawInput_->Update (lparam);
	gamePad_->Update ();
}

void InputManager::EndFrame () {
	rawInput_->EndFrame ();
}