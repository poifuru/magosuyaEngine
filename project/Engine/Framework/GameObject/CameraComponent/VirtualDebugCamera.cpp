#include "PCH.h"
#include "VirtualDebugCamera.h"
#include "GameObject.h"
#include "InputManager.h"
#include "RawInput.h"
#include "WindowsAPI.h"
#include "MathFunction.h"
#include <cmath>

void VirtualDebugCamera::Initialize() {
	VirtualCameraComponent::Initialize();
	input_ = InputManager::GetInstance();
}

void VirtualDebugCamera::Update() {
	if (!gameObject_ || !input_ || !isDebugMode_) return;
	auto& trans = gameObject_->GetTransform();

	// カメラのワールド行列を一時的に計算して、正面(Z軸)・右(X軸)の方向ベクトルを抽出する
	Matrix4x4 world = Math::MakeAffineMatrix(trans.scale, trans.rotate, trans.translate);

	Vector3 forward = { world.m[2][0], world.m[2][1], world.m[2][2] };
	forward = Math::Normalize(forward);
	Vector3 right = { world.m[0][0], world.m[0][1], world.m[0][2] };
	right = Math::Normalize(right);
	Vector3 move = { 0.0f, 0.0f, 0.0f };

	// WASDで水平移動
	if (input_->GetRawInput()->Push('W') || (GetAsyncKeyState('W') & 0x8000)) {
		move = Math::Add(move, Math::Multiply(speed_, forward));
	}
	if (input_->GetRawInput()->Push('S') || (GetAsyncKeyState('S') & 0x8000)) {
		move = Math::Subtract(move, Math::Multiply(speed_, forward));
	}
	if (input_->GetRawInput()->Push('D') || (GetAsyncKeyState('D') & 0x8000)) {
		move = Math::Add(move, Math::Multiply(speed_, right));
	}
	if (input_->GetRawInput()->Push('A') || (GetAsyncKeyState('A') & 0x8000)) {
		move = Math::Subtract(move, Math::Multiply(speed_, right));
	}
	trans.translate = Math::Add(trans.translate, move);

	// スペース / シフトで上下移動
	if (input_->GetRawInput()->Push(VK_SPACE) || (GetAsyncKeyState(VK_SPACE) & 0x8000)) {
		trans.translate.y += speed_;
	}
	if (input_->GetRawInput()->Push(VK_SHIFT) || (GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
		trans.translate.y -= speed_;
	}

	// ホイールクリック（中ボタン）ドラッグで視点回転
	if (input_->GetRawInput()->TriggerMouse(MouseButton::MIDDLE)) {
		ShowCursor(FALSE);
		RECT clientRect;
		GetClientRect(WindowsAPI::GetInstance()->GetHwnd(), &clientRect);
		POINT pt = { clientRect.left, clientRect.top };
		ClientToScreen(WindowsAPI::GetInstance()->GetHwnd(), &pt);
		clientRect.left = pt.x;
		clientRect.top = pt.y;
		pt.x = clientRect.right;
		pt.y = clientRect.bottom;
		ClientToScreen(WindowsAPI::GetInstance()->GetHwnd(), &pt);
		clientRect.right = pt.x;
		clientRect.bottom = pt.y;
		ClipCursor(&clientRect);
	}
	if (input_->GetRawInput()->ReleaseMouse(MouseButton::MIDDLE)) {
		ClipCursor(NULL);
		ShowCursor(TRUE);
	}
	bool middlePressed = input_->GetRawInput()->PushMouse(MouseButton::MIDDLE) || (GetAsyncKeyState(VK_MBUTTON) & 0x8000);
	if (middlePressed) {
		trans.rotate.y += input_->GetRawInput()->GetMouseDeltaX() * sensitivity_;
		trans.rotate.x += input_->GetRawInput()->GetMouseDeltaY() * sensitivity_;

		// 左右回転（Y軸）を 0 〜 2*PI の範囲に制限する（オーバーフロー防止）
		const float kTwoPi = 6.2831853f;
		trans.rotate.y = std::fmod(trans.rotate.y, kTwoPi);
		if (trans.rotate.y < 0.0f) {
			trans.rotate.y += kTwoPi;
		}

		// 上を向きすぎるのを防止する
		if (trans.rotate.x > pitchOver_) {
			trans.rotate.x = pitchOver_;
		}
		if (trans.rotate.x < -pitchOver_) {
			trans.rotate.x = -pitchOver_;
		}
	}
}

void VirtualDebugCamera::ImGui() {
	VirtualCameraComponent::ImGui();
	ImGui::Separator();
	ImGui::DragFloat("移動の速さ", &speed_, 0.1f, 0.01f, 10.0f);
	ImGui::DragFloat("マウス感度", &sensitivity_, 0.0001f, 0.0001f, 0.01f, "%.4f");
}

void VirtualDebugCamera::Serialize(json& j) const {
	VirtualCameraComponent::Serialize(j);
	j["type"] = "VirtualDebugCamera";
	j["speed"] = speed_;
	j["sensitivity"] = sensitivity_;
}

void VirtualDebugCamera::Deserialize(const json& j) {
	VirtualCameraComponent::Deserialize(j);
	if (j.contains("speed")) speed_ = j["speed"];
	if (j.contains("sensitivity")) sensitivity_ = j["sensitivity"];
}