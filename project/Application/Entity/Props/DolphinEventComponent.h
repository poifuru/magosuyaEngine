#pragma once
#include "RenderComponent.h"
#include "MathTypes.h"
#include <nlohmann/json.hpp>
#include <memory>
#include <string>

using json = nlohmann::json;

namespace MyEngine::Rendering {
	class Model;
}

class BoatDriverComponent;
class BoatWakeComponent;
class BoarderComponent;
class GameFlowComponent;

// イルカが船の横を並走して、航跡どうしの重なり（干渉）で大波を作るイベント
// ・どのウェイポイントで来るかは、ステージ開始時に「候補」の点から1つ抽選する（必ず1回来る）
//   候補はコースエディタで印を付ける。1つも無ければ、ゴール直前を除いた道中の点から選ぶ
// ・船がその点を通過したら Approach → Escort → Leave と進む
// ・イルカは船と同じ速さで横に lateral 離れて走り、自分の V 字の航跡を立てる
//   2つの V の内側の腕は、船の後ろ x = lateral / (2 tanθ)、横 lateral / 2 で交わる（sinθ = 波の速さ / 船の速さ）
//   その点は船の航跡のイルカ側の腕の上にあるので、ボーダーがイルカ側へ振り出せば届く
//   → 交わる点がロープの届く距離に来るように lateral を毎フレーム計算する
// ・GameFlow が Playing の間だけ動く
class DolphinEventComponent : public MyEngine::Rendering::RenderComponent {
public:
	enum class State {
		Waiting,  // 抽選した点を船が通過するのを待っている
		Approach, // 横から近づいてくる（予兆。背びれだけ見える）
		Escort,   // 船の横を並走（大波ができる。ここが跳ぶ時間）
		Leave,    // 斜め前へ離れていく
		Done,     // 終わった（このプレイではもう来ない）
	};

	DolphinEventComponent();
	~DolphinEventComponent() override;

	void Update() override;
	void ImGui() override;
	void DrawDebug() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "Dolphin Event"; }

	void CollectRenderData(MyEngine::Rendering::RenderCollector* collector) const override;

	// モデルの行列を最新のカメラで更新する（GameObject::UpdateTransformBuffer から毎フレーム呼ぶ）
	void UpdateModel();

	// ===== HUD 用 =====
	State GetState() const { return state_; }
	float GetStateTime() const { return stateTime_; }
	const std::string& GetNoticeText() const { return noticeText_; }   // カウントダウン中に出す予告
	const std::string& GetArrivalText() const { return arrivalText_; } // 近づいてくるときに出す
	int GetChosenWaypoint() const { return chosenWaypoint_; }

private:
	bool FindReferences();
	void ChooseWaypoint();      // 候補から1つ抽選する（ステージ開始時に1回）
	void ChangeState(State next);
	void StartEvent();          // Approach を始める

	// 状態ごとの「船から見た位置」（前方向 forward[m]、横 lateral[m]、深さ depth[m]）と波の強さ（0〜1）を決める
	void EvaluatePose(float& forward, float& lateral, float& depth, float& waveWeight) const;

	// 来る側を決める（StartEvent で1回）。船がこれから曲がる方向の外側にする
	// （内側にいると、ボーダーが遠心力で外へ振られて大波に届かない）
	float DecideSide() const;

	// 波紋が広がる速さ[m/s]。シミュレーションは1フレームに1回進むので、FPS によって変わる
	float CalcWaveSpeed() const;
	// 航跡の V の半分の角度 θ の tan（船が波より遅いときは V ができないので 0）
	float CalcWakeTan() const;
	// 2つの航跡が交わってほしい点の、船の中心からの後ろ向きの距離 x[m]
	// 交わる点は「船の航跡の、イルカ側の腕」の上にあるので、その腕とロープの届く円が重なる所を選ぶ
	float CalcCrossDistance() const;
	// その点で交わる横の間隔 lateral = 2 · x · tanθ
	float CalcEscortLateral() const;

private:
	// ===== 調整パラメータ（シリアライズ対象） =====
	std::string modelPath_ = "Resources/dolphin/dolphin.obj";
	std::string texturePath_ = "Resources/dolphin/dolphin_palette.png";
	float scale_ = 3.0f;              // 大きさ（モデルは全長 2.6 m）

	// --- 位置 ---
	int side_ = 0;                    // 来る側（1 = 右、-1 = 左、0 = 自動：カーブの外側。直線ならランダム）
	float waveSpeedScale_ = 1.0f;     // 波の速さの補正（デバッグ線が白い航跡とずれていたら動かす。1 = 計算どおり）
	float crossAdjust_ = 0.0f;        // 交わる点を後ろへずらす量[m]（+ で遠く、- で船の近く）
	float approachFrom_ = 40.0f;      // Approach の開始位置：並走位置よりさらに外側[m]
	float approachAhead_ = 30.0f;     // Approach の開始位置：船より前[m]
	float leaveTo_ = 50.0f;           // Leave の終わり：並走位置よりさらに外側[m]
	float leaveAhead_ = 40.0f;        // Leave の終わり：船より前[m]

	// --- 時間 ---
	float approachTime_ = 3.5f;       // 予兆の長さ[s]
	float escortTime_ = 9.0f;         // 並走の長さ[s]
	float leaveTime_ = 3.0f;          // 離れていく長さ[s]
	float fallbackDelay_ = 10.0f;     // ウェイポイントが無いコースのとき、スタートから何秒で来るか

	// --- 波 ---
	float waveStrength_ = 1.5f;       // 立てる波の強さ（船 = 1）
	float waveRadius_ = 5.0f;         // 波源の半径[m]（船は 5 m）
	float foam_ = 0.02f;              // 泡（毎フレーム）

	// --- 見た目（泳ぎ） ---
	float swimDepth_ = 0.6f;          // 並走中の深さ[m]（背びれが水面から出るくらい）
	float hiddenDepth_ = 3.0f;        // 近づき始め・離れたあとの深さ[m]
	float porpoiseHeight_ = 0.4f;     // 上下に泳ぐ幅[m]
	float porpoiseSpeed_ = 3.0f;      // 上下に泳ぐ速さ[rad/s]
	float porpoisePitch_ = 0.25f;     // 上下に合わせて頭を上げ下げする量[rad]

	// --- メッセージ（文字は Resources/Font/charset.txt に入っていること） ---
	std::string noticeText_ = "このコースのどこかで\nイルカが並走して\n大きめの波が起きるかも";
	std::string arrivalText_ = "イルカが来た！\n大きな波のチャンス";

	// ===== 実行時の状態 =====
	State state_ = State::Waiting;
	float stateTime_ = 0.0f;
	float playTime_ = 0.0f;           // Playing に入ってからの時間（ウェイポイント無しのとき用）
	bool isChosen_ = false;           // 抽選済み
	int chosenWaypoint_ = -1;         // 抽選した点（-1 = 時間で来る）
	size_t prevTarget_ = 0;           // 前フレームに船が向かっていた点
	bool hasPrevTarget_ = false;
	float sideSign_ = 1.0f;           // 今回来る側
	float swimTime_ = 0.0f;           // 上下の泳ぎの位相用
	float smoothedDt_ = 1.0f / 60.0f; // ならした dt（波の速さの計算用）

	Vector3 position_{};              // ワールド位置
	Vector2 prevXZ_{};
	bool hasPrevXZ_ = false;
	Vector2 velocity_{};              // 水平速度[m/s]（波源に渡す）
	float yaw_ = 0.0f;
	float pitch_ = 0.0f;              // + で頭が上

	// 直前の「船から見た位置」（Leave をそこから始める・デバッグ表示）
	float lastForward_ = 0.0f;
	float lastLateral_ = 0.0f;
	float lastDepth_ = 0.0f;
	float lastWaveWeight_ = 0.0f;
	float leaveFromForward_ = 0.0f;
	float leaveFromLateral_ = 0.0f;
	float leaveFromDepth_ = 0.0f;
	float leaveFromWeight_ = 0.0f;

	BoatDriverComponent* boat_ = nullptr;
	BoatWakeComponent* wake_ = nullptr;
	BoarderComponent* boarder_ = nullptr;
	GameFlowComponent* flow_ = nullptr;

	std::unique_ptr<MyEngine::Rendering::Model> model_;
	uint32_t textureIndex_ = 0;
	int32_t querySlot_ = -1;          // WaterQuery の枠（-1 = 枠なし、ゲルストナーだけで高さを取る）
};
