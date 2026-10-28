#include "PCH.h"
#include "StartupSceneSettings.h"

namespace {
	std::string currentScenePath;
}

std::string StartupSceneSettings::LoadStartupScenePath() {
	if (!std::filesystem::exists(kSettingsFilePath)) {
		return kDefaultScenePath;
	}
	std::ifstream file(kSettingsFilePath);
	if (!file.is_open()) {
		Logger::GetInstance()->Log("StartupSceneSettings: 設定ファイルを開けなかったのでデフォルトシーンを使います");
		return kDefaultScenePath;
	}

	// 壊れたJSONでも落ちないように例外を出さずにパースする
	nlohmann::json root = nlohmann::json::parse(file, nullptr, false);
	if (root.is_discarded() || !root.contains("startupScene") || !root["startupScene"].is_string()) {
		Logger::GetInstance()->Log("StartupSceneSettings: 設定ファイルが不正なのでデフォルトシーンを使います");
		return kDefaultScenePath;
	}

	std::string scenePath = root["startupScene"].get<std::string>();
	if (!std::filesystem::exists(scenePath)) {
		Logger::GetInstance()->Log("StartupSceneSettings: " + scenePath + " が見つからないのでデフォルトシーンを使います");
		return kDefaultScenePath;
	}
	return scenePath;
}

void StartupSceneSettings::SaveStartupScenePath(const std::string& scenePath) {
	nlohmann::json root;
	root["startupScene"] = scenePath;
	std::filesystem::path path(kSettingsFilePath);
	if (path.has_parent_path() && !std::filesystem::exists(path.parent_path())) {
		std::filesystem::create_directories(path.parent_path());
	}
	std::ofstream file(kSettingsFilePath);
	if (file.is_open()) {
		file << root.dump(4);
	}
}

void StartupSceneSettings::SetCurrentScenePath(const std::string& scenePath) {
	currentScenePath = scenePath;
}

const std::string& StartupSceneSettings::GetCurrentScenePath() {
	return currentScenePath;
}
