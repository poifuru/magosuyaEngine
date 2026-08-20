#pragma once
#include "Component.h"

class ReticleComponent : public Component {
public:
	ReticleComponent() = default;
	~ReticleComponent() override = default;

	void Initialize() override;
	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "ReticleComponent"; }

	// ロックオンしている敵オブジェクトを返すゲッター
	GameObject* GetLockOnTarget() const { return lockOnTarget_; }

	// ロックオン有効最大距離（ハープーン射程）の設定
	void SetLockOnMaxDistance(float dist) { lockOnMaxDistance_ = dist; }
	float GetLockOnMaxDistance() const { return lockOnMaxDistance_; }

	void SetVisible(bool visible) { isVisible_ = visible; }
	bool IsVisible() const { return isVisible_; }

private:
	Vector2 spriteSize_ = { 64.0f, 64.0f };   // スプライトのサイズ（幅・高さ）
	Vector2 spriteOffset_ = { 0.0f, 0.0f };   // 画面中央からの位置微調整（X・Y）
	Vector2 spriteScale_ = { 1.0f, 1.0f };    // スケール倍率
	float lockOnAngleCos_ = 0.995f; // ロックオンのしきい値
	float lockOnMaxDistance_ = 18.0f; // ロックオン有効最大距離（ハープーン射程）
	Vector4 normalColor_ = { 1.0f, 1.0f, 1.0f, 1.0f }; // 通常時の色
	Vector4 lockOnColor_ = { 1.0f, 0.1f, 0.1f, 1.0f }; // ロックオン時の色
	bool isInitialized_ = false; // 初期化済みフラグ
	bool isVisible_ = true;		// 表示・非表示を明示的に切り替えるフラグ

	GameObject* lockOnTarget_ = nullptr; // 現在ロックオンしている敵
};