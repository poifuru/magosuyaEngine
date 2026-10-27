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

	// --- なめらかな追従 ---
	// カメラの位置も注視点も「なめらかにしたターゲット位置」を基準にする（生の位置だと波の上下で首を振る）
	// 半減期 = 目標との差が半分になるまでの秒数。dt で計算するので FPS に左右されない
	float xzHalfLife_ = 0.02f;      // 水平の追従（船が速いので、遅らせると置いていかれる）[s]
	float ySmoothTime_ = 0.45f;     // 高さの追従（波の上下を吸収する）[s]
	float airYSmoothTime_ = 0.15f;  // ジャンプ中の高さの追従（遅すぎると画面の上に抜ける）[s]
	Vector3 smoothedTarget_{};      // なめらかにしたターゲット位置
	float smoothedYVel_ = 0.0f;     // SmoothDamp 用の高さの速度
	bool hasSmoothedTarget_ = false;

	// --- ボートの進行方向に合わせる ---
	bool followBoatHeading_ = true; // ON：カメラの向き（Yaw）をボートに合わせる
	bool alignToRope_ = true;       // ON：ボートとプレイヤーを結んだ線の延長上にカメラを置く（OFF：ボートの進行方向の真後ろ）
	float minRopeLength_ = 1.0f;    // ボートとプレイヤーがこれより近いときは、ボートの進行方向を使う[m]
	float headingHalfLife_ = 0.35f; // 向きの追従[s]
	float smoothedHeading_ = 0.0f;  // なめらかにしたボートの向き
	bool hasHeading_ = false;

	// 一人称視点（FPS）用の設定
	bool isFirstPerson_ = false;              // 一人称視点フラグ
	Vector3 firstPersonOffset_ = { 0.0f, 1.2f, 0.0f }; // 一人称時の視点オフセット（自機中心より少し上）

	// カメラの回転角（プレイヤーは操作しない。インスペクタで決める）
	float angleX_ = 0.2f; // 上下回転（Pitch: 最初は少し見下ろす角度にする）
	float angleY_ = 0.0f; // 左右回転（Yaw）。ボートの向きに合わせる間は「進行方向からのずれ」（0 で真後ろ）

	// 水中フェーズフラグ
	bool isUnderwater_ = false;
};