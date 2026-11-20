#pragma once
#include "IEditorWindow.h"
#include "MathTypes.h"
#include "GpuParticleSystem.h"   // ParticleEmitTimer をメンバに持つので前方宣言では足りない
#include <set>
#include <string>

namespace MyEngine::Rendering {
	class Renderer;
}

struct ParticleEffectData;
struct CameraData;

// GPUパーティクルのエフェクト（Resources/Particle/*.json）を編集するウィンドウ
// ・左：エフェクトの一覧・新規作成・保存
// ・右上：プレビュー（位置・バースト再生・生きている粒の数）
// ・右下：パラメータ（値を動かすとその場で見た目に反映される）
class ParticleEditorWindow : public IEditorWindow {
public:
	ParticleEditorWindow();
	~ParticleEditorWindow() override = default;

	void UpdateAndDraw(const EditorContext& context) override;

	// パーティクルのワークスペース用：今のウィンドウ（ホスト）の中に、画面いっぱいで中身を描く
	void DrawWorkspace(const EditorContext& context);

	CameraData MakePreviewCamera() const;

private:
	// ウィンドウの中身（通常のウィンドウとワークスペースの両方から呼ぶ）
	void DrawContents(const EditorContext& context);

	// 左：一覧・新規作成・保存（6-5）
	void DrawEffectList(GpuParticleSystem& system);

	// 右上：プレビュー（6-5）
	void DrawPreview(GpuParticleSystem& system, MyEngine::Rendering::Renderer* renderer);

	// 右下：パラメータ。値が変わったフレームだけ true を返す（6-4）
	bool DrawParameters(GpuParticleSystem& system, ParticleEffectData& data);

private:
	std::string selectedName_;                        // 編集中のエフェクト
	Vector3 previewPosition_ = { 0.0f, 2.0f, 0.0f };  // プレビュー位置
	char newNameBuffer_[64] = "";                     // 新規作成の名前入力
	std::set<std::string> unsavedNames_;              // 保存していないエフェクト
	std::string message_;                             // 新規作成・保存の結果（一覧の下に出す）
	bool previewLoop_ = true;                         // 「出し続ける」エフェクトをプレビュー位置から出す
	ParticleEmitTimer previewTimer_;                  // プレビューの放出数の計算用

	// プレビュー用のカメラ
	float orbitYaw_ = 0.6f;       // 横の角度（ラジアン）
	float orbitPitch_ = 0.35f;    // 縦の角度（ラジアン）
	float orbitDistance_ = 8.0f;  // 中心からの距離

	// プレビューのグリッド
	bool showGrid_ = true;        // グリッドを出すか
	float gridSize_ = 10.0f;      // 一辺の長さ
	float gridStep_ = 1.0f;       // 線の間隔

	// プレビューにグリッドと中心の軸を出す（線の要求だけ。描くのは Renderer）
	void RequestPreviewGrid() const;
};
