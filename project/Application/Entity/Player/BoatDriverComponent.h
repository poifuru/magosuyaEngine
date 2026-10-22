#pragma once
#include "Component.h"
#include "MathTypes.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// 船を自動で走らせるコンポーネント
// ・スタートから acceleration_ で加速し、cruiseSpeed_ で一定速度になる
// ・今は向きを変えずにまっすぐ進む（ウェイポイント走行は次の段階）
// ・速度は m/s で持ち、dt を掛けて動かすので FPS に左右されない
class BoatDriverComponent : public Component {
public:
	BoatDriverComponent() = default;
	~BoatDriverComponent() override = default;

	void Initialize() override;
	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "BoatDriverComponent"; }

	float GetSpeed() const { return speed_; }   // 今の速度[m/s]
	float GetYaw() const { return yaw_; }       // 進行方向[rad]

	// スタート地点に戻して、速度0からやり直す
	void ResetToStart();

private:
	// ===== 調整パラメータ（シリアライズ対象） =====
	float cruiseSpeed_ = 10.0f;  // 巡航速度[m/s]
	float acceleration_ = 3.0f;  // 加速[m/s²]（スタート時に徐々に速くなる）
	float boatHeight_ = 0.3f;    // 船の高さ（水面に置く y）
	bool isRunning_ = true;      // false で停止（エディタでの確認用）

	// ===== 実行中の状態 =====
	float speed_ = 0.0f;         // 今の速度[m/s]
	float yaw_ = 0.0f;           // 進行方向[rad]
	float distance_ = 0.0f;      // スタートからの走行距離[m]

	// スタート地点（Initialize 時の位置と向きを覚えておく）
	Vector3 startPos_{};
	float startYaw_ = 0.0f;
};
