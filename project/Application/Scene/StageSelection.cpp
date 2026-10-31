#include "PCH.h"
#include "StageSelection.h"
#include "GameFlowComponent.h"
#include "DolphinEventComponent.h"
#include <unordered_map>

namespace {
	bool hasSelection = false;
	int selectedIndex = 0;
	std::string selectedScenePath;

	std::unordered_map<std::string, float> bestHeights;

	// シーンjsonの中から GameFlow と DolphinEvent を探して、ステージ情報を埋める
	void ReadSceneInfo(StageInfo& stage) {
		stage.sceneExists = std::filesystem::exists(stage.scenePath);
		if (!stage.sceneExists) return;

		std::ifstream file(stage.scenePath);
		if (!file.is_open()) return;
		nlohmann::json sceneJ = nlohmann::json::parse(file, nullptr, false);
		if (sceneJ.is_discarded() || !sceneJ.contains("objects")) return;

		for (const auto& objJ : sceneJ["objects"]) {
			if (!objJ.contains("components")) continue;
			for (const auto& compJ : objJ["components"]) {
				const std::string type = compJ.value("type", "");
				// キーが無いときはコンポーネント側の初期値と同じ値にする
				if (type == "GameFlowComponent") {
					stage.maxJumps = compJ.value("maxJumps", GameFlowComponent::kDefaultMaxJumps);
				}
				else if (type == "DolphinEventComponent") {
					stage.hasDolphin = true;
					stage.dolphinNotice = compJ.value("noticeText", std::string(DolphinEventComponent::kDefaultNoticeText));
				}
			}
		}
	}
}

std::vector<StageInfo> StageSelection::LoadStageList() {
	std::vector<StageInfo> stages;

	std::ifstream file(kStageListPath);
	if (!file.is_open()) {
		Logger::GetInstance()->Log("StageSelection: " + kStageListPath + " が開けませんでした");
		return stages;
	}

	// 壊れたJSONでも落ちないように例外を出さずにパースする
	nlohmann::json root = nlohmann::json::parse(file, nullptr, false);
	if (root.is_discarded() || !root.contains("stages") || !root["stages"].is_array()) {
		Logger::GetInstance()->Log("StageSelection: " + kStageListPath + " の形式が不正です");
		return stages;
	}

	for (const auto& stageJ : root["stages"]) {
		StageInfo stage;
		stage.name = stageJ.value("name", "NO NAME");
		stage.scenePath = stageJ.value("scene", "");
		stage.description = stageJ.value("description", "");
		ReadSceneInfo(stage);
		if (!stage.sceneExists) {
			Logger::GetInstance()->Log("StageSelection: " + stage.scenePath + " が見つかりません（" + stage.name + "）");
		}
		stages.push_back(std::move(stage));
	}
	return stages;
}

void StageSelection::Select(int index, const StageInfo& stage) {
	hasSelection = true;
	selectedIndex = index;
	selectedScenePath = stage.scenePath;
}

void StageSelection::Clear() {
	hasSelection = false;
	selectedScenePath.clear();
}

bool StageSelection::HasSelection() {
	return hasSelection;
}

const std::string& StageSelection::GetSelectedScenePath() {
	return selectedScenePath;
}

int StageSelection::GetSelectedIndex() {
	return selectedIndex;
}

float StageRecords::GetBestHeight(const std::string& scenePath) {
	auto it = bestHeights.find(scenePath);
	return (it != bestHeights.end()) ? it->second : 0.0f;
}

bool StageRecords::SubmitHeight(const std::string& scenePath, float height) {
	float& best = bestHeights[scenePath];
	if (height <= best) return false;
	best = height;
	return true;
}

void StageRecords::Reset(const std::string& scenePath) {
	bestHeights.erase(scenePath);
}
