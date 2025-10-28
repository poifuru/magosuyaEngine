#include "InputManager.h"

std::unique_ptr<InputManager> g_inputManager = nullptr;

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