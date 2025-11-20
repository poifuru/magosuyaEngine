#pragma once

class ImGuiManager {
public:
	static ImGuiManager* GetInstance () {
		//初めて呼び出されたときに一回だけ初期化
		static ImGuiManager instance;
		return &instance;
	}
	~ImGuiManager ();

	void Initialize ();
	void Draw ();
	void BeginFrame ();

private:
	//コンストラクタを禁止
	ImGuiManager () = default;
	// コピーコンストラクタと代入演算子を禁止
	ImGuiManager (const ImGuiManager&) = delete;
	ImGuiManager& operator=(const ImGuiManager&) = delete;
	ImGuiManager (ImGuiManager&&) = delete;
	ImGuiManager& operator=(ImGuiManager&&) = delete;
};

