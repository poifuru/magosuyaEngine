#include "DebugCamera.h"

DebugCamera::DebugCamera() {
	transform_ = {};
	worldMatrix_ = {};
	viewMatrix_ = {};
	projectionMatrix_ = {};

	forward_ = {};
	right_ = {};
	speed_ = 0.0f;
	move_ = {};

	sensitivity_ = 0.001f;
	pitchOver_ = 1.5708f;
}

void DebugCamera::Initialize() {
	transform_ = {
		{1.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, -10.0f},
	};
	worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	viewMatrix_ = Inverse(worldMatrix_);
	projectionMatrix_ = MakePerspectiveFOVMatrix(0.45f, 1280.0f / 720.0f, 0.1f, 100.0f);

	speed_ = 0.1f;
}

void DebugCamera::Updata(HWND hwnd, HRESULT hr, InputManager* inputManager) {
	//=======更新処理=======//
	//カメラの前後左右の移動
	forward_ = {
	worldMatrix_.m[2][0], // z軸のx成分
	worldMatrix_.m[2][1], // z軸のy成分
	worldMatrix_.m[2][2]  // z軸のz成分
	};
	forward_ = Normalize(forward_);

	right_ = {
	worldMatrix_.m[0][0], // x軸のx成分
	worldMatrix_.m[0][1], // x軸のy成分
	worldMatrix_.m[0][2]  // x軸のz成分
	};
	right_ = Normalize(right_);

	move_ = { 0.0f, 0.0f, 0.0f };

	if (inputManager->GetRawInput()->Push('W')) {
		move_ += forward_ * speed_;
	}
	if (inputManager->GetRawInput ()->Push ('S')) {
		move_ -= forward_ * speed_;
	}
	if (inputManager->GetRawInput ()->Push ('D')) {
		move_ += right_ * speed_;
	}
	if (inputManager->GetRawInput ()->Push ('A')) {
		move_ -= right_ * speed_;
	}

	transform_.translate += move_;

	if (inputManager->GetRawInput ()->Push (VK_SPACE)) {
		transform_.translate.y += speed_;
	}
	if (inputManager->GetRawInput ()->Push (VK_SHIFT)) {
		transform_.translate.y -= speed_;
	}

	//マウスで視点移動
	//回転処理(左クリックしながらドラッグ)
	// カーソル非表示
	if (inputManager->GetRawInput ()->TriggerMouse(MouseButton::LEFT)) {
		ShowCursor(FALSE);

		// クライアント領域の矩形を取得
		RECT clientRect;
		GetClientRect (hwnd, &clientRect);

		// クライアント領域の座標をスクリーン座標に変換するでやんす
		// ClipCursorはスクリーン座標を要求するからでやんす
		POINT pt = { clientRect.left, clientRect.top };
		ClientToScreen (hwnd, &pt);
		clientRect.left = pt.x;
		clientRect.top = pt.y;

		pt.x = clientRect.right;
		pt.y = clientRect.bottom;
		ClientToScreen (hwnd, &pt);
		clientRect.right = pt.x;
		clientRect.bottom = pt.y;

		// カーソルをウィンドウのクライアント領域に制限するでやんす！
		ClipCursor (&clientRect);
	}
	if (inputManager->GetRawInput ()->ReleaseMouse (MouseButton::LEFT)) {
		// カーソルの制限を解除（NULLを指定）
		ClipCursor (NULL);
		ShowCursor (TRUE);
	}

	if (inputManager->GetRawInput ()->PushMouse (MouseButton::LEFT)) {
		transform_.rotate.y += inputManager->GetRawInput ()->GetMouseDeltaX() * sensitivity_;
		transform_.rotate.x += inputManager->GetRawInput ()->GetMouseDeltaY () * sensitivity_;

		if (transform_.rotate.x > pitchOver_) {
			transform_.rotate.x = pitchOver_;
		}
		if (transform_.rotate.x < -pitchOver_) {
			transform_.rotate.x = -pitchOver_;
		}
	}

	//変化した情報をworldMatrixにまとめてviewMatrixに入れる
	worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	viewMatrix_ = Inverse(worldMatrix_);
}