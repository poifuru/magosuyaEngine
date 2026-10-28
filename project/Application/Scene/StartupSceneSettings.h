#pragma once

// 起動時に最初に読み込むシーンファイルの設定（Releaseでも読み込みは動く）
namespace StartupSceneSettings {
	// シーンファイルを置いているフォルダ
	inline const std::string kSceneFolder = "Resources/Scene";

	// 設定ファイルが無い・壊れている・指定先が無いときに使うシーン
	inline const std::string kDefaultScenePath = "Resources/Scene/defaultScene.json";

	// 設定ファイルの保存先
	inline const std::string kSettingsFilePath = "Resources/Editor/startup_settings.json";

	// 起動シーンのパスを取得（失敗時は警告ログを出してデフォルトを返す）
	std::string LoadStartupScenePath();

	// 起動シーンのパスを保存
	void SaveStartupScenePath(const std::string& scenePath);

	// 現在開いているシーンファイルのパス（エディタの「現在のシーンを起動シーンに」用）
	void SetCurrentScenePath(const std::string& scenePath);
	const std::string& GetCurrentScenePath();
}
