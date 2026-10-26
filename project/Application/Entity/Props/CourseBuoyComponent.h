#pragma once
#include "RenderComponent.h"
#include "MathTypes.h"
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace MyEngine::Rendering {
	class Model;
}

// コースのウェイポイントにウキ（ブイ）のモデルを並べるコンポーネント
// ・同じ GameObject の BoatDriverComponent のウェイポイントを読んで置く
//   （道中の点：その点に1つ、ゴールの点：ゲートの左右に1つずつ）
// ・高さは水面（WaterSurfaceComponent）のゲルストナー波に合わせて浮かせる
// ・モデルは点の数に合わせて足りない分だけ作り、余った分は描かない（プール）
class CourseBuoyComponent : public MyEngine::Rendering::RenderComponent {
public:
	CourseBuoyComponent();
	~CourseBuoyComponent() override;

	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "Course Buoys"; }

	void CollectRenderData(MyEngine::Rendering::RenderCollector* collector) const override;

private:
	// モデル1つぶんの置き場所
	struct Placement {
		Vector3 pos;
		float yaw;
	};

	// モデルを1つ作る（失敗したら nullptr）
	std::unique_ptr<MyEngine::Rendering::Model> CreateBuoyModel(const std::string& path);

	// プールの数を count 以上にして、先頭から placements の位置に置く
	void PlaceModels(std::vector<std::unique_ptr<MyEngine::Rendering::Model>>& pool, size_t& activeCount,
		const std::string& path, const std::vector<Placement>& placements);

	// --- 設定値（シリアライズ対象） ---
	std::string passModelPath_ = "Resources/Props/Buoy/buoy_waypoint.obj"; // 道中のウキ
	std::string goalModelPath_ = "Resources/Props/Buoy/buoy_goal.obj";     // ゴールのウキ
	std::string texturePath_ = "Resources/Props/Buoy/buoy_palette.png";    // 色パレット
	float buoyScale_ = 2.0f;     // 大きさ（モデルは浮きの直径 1.26 m、旗の先が 3.2 m）
	bool floatOnWater_ = true;   // 水面の波に合わせて浮かせる
	float heightOffset_ = 0.0f;  // 浮かせる高さの調整 [m]
	bool flipFlag_ = false;      // 旗の向きを反対にする（ゲートの旗が外を向いていたら ON）
	float swayAmount_ = 0.06f;   // ゆらゆら傾く量 [rad]

	// --- 状態 ---
	std::vector<std::unique_ptr<MyEngine::Rendering::Model>> passModels_;
	std::vector<std::unique_ptr<MyEngine::Rendering::Model>> goalModels_;
	size_t passActive_ = 0;      // 今描いている数
	size_t goalActive_ = 0;
	uint32_t textureIndex_ = 0;
	bool textureLoaded_ = false;
	float time_ = 0.0f;          // 揺れ用
};
