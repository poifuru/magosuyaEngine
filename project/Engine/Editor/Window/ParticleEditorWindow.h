#pragma once
#include "IEditorWindow.h"
#include "MathTypes.h"
#include "GpuParticleSystem.h"   // ParticleEmitTimer をメンバに持つので前方宣言では足りない
#include <set>
#include <string>

struct ParticleEffectData;

// GPUパーティクルのエフェクト（Resources/Particle/*.json）を編集するウィンドウ
// ・左：エフェクトの一覧・新規作成・保存
// ・右上：プレビュー（位置・バースト再生・生きている粒の数）
// ・右下：パラメータ（値を動かすとその場で見た目に反映される）
class ParticleEditorWindow : public IEditorWindow {
public:
	ParticleEditorWindow();
	~ParticleEditorWindow() override = default;

	void UpdateAndDraw(const EditorContext& context) override;

private:
	// 左：一覧・新規作成・保存（6-5）
	void DrawEffectList(GpuParticleSystem& system);

	// 右上：プレビュー（6-5）
	void DrawPreview(GpuParticleSystem& system);

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
};
