#pragma once
#include "MagosuyaEngine.h"
#include "object/3d/Model.h"

class Boss;

enum class StompPhase {
	None,       // 何もしていない
	Rise,       // 中央上空へ移動中
	Hover,      // 上空で待機（予兆表示）
	Fall,       // 急降下（攻撃）
	Cooldown    // 着地後の硬直
};

class CenterStomp {
public:
	CenterStomp(MagosuyaEngine* magosuya, Boss* boss);
	~CenterStomp();

	void Initialize();
	void Update(Matrix4x4* m);
	void Draw();
	void ImGuiControl();

	void StartAttack();
	bool IsAttacking() const { return phase_ != StompPhase::None; }
private:
	// 各フェーズの処理
	void UpdateRise();   // 上昇
	void UpdateHover();  // 待機
	void UpdateFall();   // 落下
	void UpdateCooldown(); // 硬直
private:
	MagosuyaEngine* magosuya_ = nullptr;
	std::unique_ptr<Model> model_ = nullptr;

	// ボス攻撃時の波
	Transform transform_;
	bool isAliveWave_ = false;

	// ボスの攻撃系
	Boss* boss_ = nullptr;
	StompPhase phase_ = StompPhase::None;
	int timer_ = 0;           // 汎用タイマー
	int duration_ = 0;        // 目標時間
	Vector3 startPos_;        // 移動開始地点
	Vector3 targetPos_;       // 移動目標地点
	const Vector3 kCenterPoint_ = {0.0f, 0.0f, 0.0f};
	const float kHoverHeight_ = 10.0f;
};

