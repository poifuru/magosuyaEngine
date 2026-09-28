#pragma once
#include "VirtualCameraComponent.h"

class VirtualFollowCamera : public VirtualCameraComponent {
public:
	VirtualFollowCamera() = default;
	~VirtualFollowCamera() override;

	void Update() override;
	void ImGui() override;

	void ResolveTarget(const std::vector<std::unique_ptr<GameObject>>& gameObjects);

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	// アクセッサ
	const char* GetName() const override { return "Virtual Follow Camera"; }

	bool IsFirstPerson() const { return isFirstPerson_; }
	void SetFirstPerson(bool flag);

	bool IsUnderwater() const { return isUnderwater_; }
	void SetUnderwater(bool flag) { isUnderwater_ = flag; }

private:
	void UpdateTargetVisibility();

private:
	std::string targetName_ = "";
	GameObject* target_ = nullptr;

	Vector3 offset_ = { 0.0f, 6.5f, -28.0f }; // ターゲットからの距離 (高さを少し上げて見下ろし角を調整)
	float targetOffsetY_ = 3.8f;              // 注視点の高さオフセット (画面中央と自機の被りを防ぐ)
	float delay_ = 0.1f;                      // 追従の遅延 (0 = 遅延なし, 1 = 動かない)

	// 一人称視点（FPS）用の設定
	bool isFirstPerson_ = false;              // 一人称視点フラグ
	Vector3 firstPersonOffset_ = { 0.0f, 1.2f, 0.0f }; // 一人称時の視点オフセット（自機中心より少し上）

	// カメラの回転角
	float angleX_ = 0.2f; // 上下回転（Pitch: 最初は少し見下ろす角度にする）
	float angleY_ = 0.0f; // 左右回転（Yaw）

	// 水中フェーズフラグ
	bool isUnderwater_ = false;
};