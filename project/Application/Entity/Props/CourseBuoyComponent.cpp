#include "PCH.h"
#include "CourseBuoyComponent.h"
#include "BoatDriverComponent.h"
#include "WaterSurfaceComponent.h"
#include "GameObject.h"
#include "BaseScene.h"       // SceneContext を使うため
#include "ModelFactory.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "CameraOrganizer.h"
#include "RenderCollector.h"
#include "Model.h"
#include "MeshData.h"
#include "Time.h"

#include <cmath>
#include <numbers>

CourseBuoyComponent::CourseBuoyComponent() = default;
CourseBuoyComponent::~CourseBuoyComponent() = default;

std::unique_ptr<MyEngine::Rendering::Model> CourseBuoyComponent::CreateBuoyModel(const std::string& path) {
	GameObject* owner = GetGameObject();
	if (!owner) return nullptr;
	SceneContext* context = owner->GetContext();
	if (!context) return nullptr;

	// テクスチャは最初の1回だけ読む（2回目以降は TextureManager が同じ番号を返す）
	if (!textureLoaded_) {
		textureIndex_ = context->textureManager->LoadTexture(texturePath_);
		textureLoaded_ = true;
	}

	uint32_t modelIndex = context->modelManager->LoadModelData(path);
	return context->modelFactory->CreateModel(modelIndex, textureIndex_);
}

void CourseBuoyComponent::PlaceModels(std::vector<std::unique_ptr<MyEngine::Rendering::Model>>& pool, size_t& activeCount,
	const std::string& path, const std::vector<Placement>& placements) {
	// 足りない分だけ作る（減ったときは消さずに残しておき、描かないだけ）
	while (pool.size() < placements.size()) {
		auto model = CreateBuoyModel(path);
		if (!model) break;
		pool.push_back(std::move(model));
	}
	activeCount = std::min(pool.size(), placements.size());

	auto* water = gameObject_->FindComponentInScene<WaterSurfaceComponent>();
	const CameraData& cameraData = CameraOrganizer::GetInstance()->GetCameraData();

	for (size_t i = 0; i < activeCount; ++i) {
		const Placement& pl = placements[i];
		Vector3 pos = pl.pos;
		pos.y = heightOffset_;
		if (floatOnWater_ && water) {
			pos.y += water->SampleGerstnerHeight(pos.x, pos.z);
		}

		// ウキごとに位相をずらして、ゆらゆら傾ける
		float phase = time_ * 1.3f + static_cast<float>(i) * 1.7f + pos.x * 0.05f;
		Vector3 rotate = { std::sin(phase) * swayAmount_, pl.yaw, std::cos(phase * 0.8f) * swayAmount_ };

		auto& model = pool[i];
		model->SetTranslate(pos);
		model->SetRotate(rotate);
		model->SetScale({ buoyScale_, buoyScale_, buoyScale_ });
		model->Update(&cameraData);
	}
}

void CourseBuoyComponent::Update() {
	if (!gameObject_) return;
	auto* driver = gameObject_->GetComponent<BoatDriverComponent>();
	if (!driver) {
		passActive_ = goalActive_ = 0;
		return;
	}

	// 揺れの時間（ポーズ中は dt が 0 なので止まる）
	time_ += Time::GetDeltaTime();

	// 置く場所を集める
	std::vector<Placement> passPlacements;
	std::vector<Placement> goalPlacements;
	const auto& wps = driver->GetWaypoints();
	const float flip = flipFlag_ ? std::numbers::pi_v<float> : 0.0f;
	for (size_t i = 0; i < wps.size(); ++i) {
		Vector3 dir = driver->GetIncomingDir(i);
		float yaw = std::atan2(dir.x, dir.z); // 船と同じ向きの決め方

		if (wps[i].type == BoatDriverComponent::WaypointType::Goal) {
			// ゲートの左右に置く。旗が内側（相手のウキ）を向くように、右は半回転させる
			Vector3 left, right;
			driver->GetGoalGate(i, left, right);
			goalPlacements.push_back({ left, yaw + flip });
			goalPlacements.push_back({ right, yaw + flip + std::numbers::pi_v<float> });
		} else {
			passPlacements.push_back({ wps[i].pos, yaw + flip });
		}
	}

	PlaceModels(passModels_, passActive_, passModelPath_, passPlacements);
	PlaceModels(goalModels_, goalActive_, goalModelPath_, goalPlacements);
}

void CourseBuoyComponent::ImGui() {
#ifdef USEIMGUI
	ImGui::Checkbox("Visible", &isVisible_);
	ImGui::DragFloat("Scale", &buoyScale_, 0.05f, 0.1f, 20.0f);
	ImGui::Checkbox("Float On Water", &floatOnWater_);
	ImGui::DragFloat("Height Offset", &heightOffset_, 0.01f, -5.0f, 5.0f, "%.2f m");
	ImGui::DragFloat("Sway", &swayAmount_, 0.005f, 0.0f, 0.5f, "%.3f rad");
	ImGui::Checkbox("Flip Flag", &flipFlag_);
	ImGui::TextDisabled("buoys: pass %d / goal %d", static_cast<int>(passActive_), static_cast<int>(goalActive_));
	if (!gameObject_ || !gameObject_->GetComponent<BoatDriverComponent>()) {
		ImGui::TextColored({ 1.0f, 0.5f, 0.3f, 1.0f }, "Boat Driver が同じオブジェクトにありません");
	}
#endif
}

void CourseBuoyComponent::CollectRenderData(MyEngine::Rendering::RenderCollector* collector) const {
	if (!isVisible_ || !collector) return;

	auto submit = [&](const std::vector<std::unique_ptr<MyEngine::Rendering::Model>>& pool, size_t activeCount) {
		for (size_t m = 0; m < activeCount; ++m) {
			const auto& model = pool[m];
			auto* modelData = model->GetModelData();
			auto* material = model->GetMaterial();
			if (!modelData || !material) continue;

			for (size_t i = 0; i < modelData->meshes.size(); ++i) {
				MyEngine::Rendering::RenderItem item{};
				item.mesh = &modelData->meshes[i];
				item.material = material;
				item.transformAddr = model->GetMeshTransformGPUAddress(static_cast<uint32_t>(i));
				item.customAddr = 0;
				item.debugName = "CourseBuoy";
				collector->Submit(item);
			}
		}
	};
	submit(passModels_, passActive_);
	submit(goalModels_, goalActive_);
}

void CourseBuoyComponent::Serialize(json& j) const {
	j["type"] = "CourseBuoyComponent";
	j["isVisible"] = isVisible_;
	j["buoyScale"] = buoyScale_;
	j["floatOnWater"] = floatOnWater_;
	j["heightOffset"] = heightOffset_;
	j["flipFlag"] = flipFlag_;
	j["swayAmount"] = swayAmount_;
}

void CourseBuoyComponent::Deserialize(const json& j) {
	if (j.contains("isVisible")) isVisible_ = j["isVisible"];
	if (j.contains("buoyScale")) buoyScale_ = j["buoyScale"];
	if (j.contains("floatOnWater")) floatOnWater_ = j["floatOnWater"];
	if (j.contains("heightOffset")) heightOffset_ = j["heightOffset"];
	if (j.contains("flipFlag")) flipFlag_ = j["flipFlag"];
	if (j.contains("swayAmount")) swayAmount_ = j["swayAmount"];
}
