#pragma once

// ステージの一覧（Resources/Stage/stages.json）と、ステージセレクトで選んだステージを覚えておく
// ・stages.json には「名前・シーンjson・説明文」だけを書く
// ・ジャンプ回数とイルカの有無はシーンjsonの中身（GameFlow / DolphinEvent）から読む（同じ値を2か所に書かないため）
// ・選んだステージはシーンをまたいで残るように static に持つ（アプリを閉じると消える）
struct StageInfo {
	// ===== stages.json に書く値 =====
	std::string name;        // 画面に出す名前
	std::string scenePath;   // 読み込むシーンjson
	std::string description; // 説明文（改行は \n）

	// ===== シーンjsonから読む値 =====
	bool sceneExists = false; // シーンjsonがあるか（無ければ選べない）
	int maxJumps = 0;         // GameFlowComponent の maxJumps（GameFlow が無ければ 0）
	bool hasDolphin = false;  // DolphinEventComponent があるか
	std::string dolphinNotice; // イルカの予告文
};

namespace StageSelection {
	// ステージ一覧の置き場所
	inline const std::string kStageListPath = "Resources/Stage/stages.json";

	// stages.json を読んで、各ステージのシーンjsonも覗いて情報を埋める（壊れていても落ちない）
	std::vector<StageInfo> LoadStageList();

	// ステージを選ぶ（index はステージセレクトに戻ったときのカーソル位置用）
	void Select(int index, const StageInfo& stage);
	// 選択を解除する（PlayScene は起動シーン設定のシーンを読むようになる）
	void Clear();

	bool HasSelection();
	const std::string& GetSelectedScenePath();
	int GetSelectedIndex(); // 選んでいなければ 0
}

// ステージ別の自己ベスト（キーはシーンjsonのパス。アプリを閉じると消える）
namespace StageRecords {
	float GetBestHeight(const std::string& scenePath);
	// 高さを記録する。自己ベストを更新したら true
	bool SubmitHeight(const std::string& scenePath, float height);
	void Reset(const std::string& scenePath);
}
