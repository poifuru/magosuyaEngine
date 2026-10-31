#pragma once
#include "Component.h"
#include "StageSelection.h"
#include <nlohmann/json.hpp>
#include <vector>

using json = nlohmann::json;

class FontTextComponent;

// ステージセレクトの操作と表示
// ・同じ GameObject の FontText にステージ一覧を出す
// ・detailObjectName_ という名前の GameObject の FontText に、選んでいるステージの詳細を出す
//   （ジャンプ回数・自己ベスト・説明文・イルカの予告）
// ・上下で選択、A / Space で PlayScene へ、B / BackSpace で TitleScene へ
class StageSelectComponent : public Component {
public:
	StageSelectComponent() = default;
	~StageSelectComponent() override = default;

	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "StageSelectComponent"; }

private:
	void LoadStages();
	void FindReferences();
	void UpdateInput();
	void MoveCursor(int delta);
	void Decide();
	void UpdateText();

	bool CanAcceptInput() const; // 再生中で、シーン遷移中でないか

private:
	// ===== 調整パラメータ（シリアライズ対象） =====
	std::string detailObjectName_ = "StageDetail"; // 詳細を出す FontText を持つ GameObject の名前
	float stickThreshold_ = 0.5f;                  // スティックをこれ以上倒したら1つ動く

	// ===== 実行時の状態 =====
	std::vector<StageInfo> stages_;
	bool isLoaded_ = false;
	int cursor_ = 0;
	int prevStickDir_ = 0; // 前フレームのスティックの向き（-1 下 / 0 / 1 上）。倒しっぱなしで連続移動しないように

	FontTextComponent* listText_ = nullptr;   // 同じ GameObject の FontText
	FontTextComponent* detailText_ = nullptr; // detailObjectName_ の FontText（無ければ一覧の下にまとめて出す）
};
