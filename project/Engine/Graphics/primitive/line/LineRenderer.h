#pragma once
#include "MathTypes.h"
#include "ConstantBuffer.h"
#include "LineRenderData.h"

namespace MyEngine::Rendering {
	class PSOManager;
	class ShaderManager;
	class InputLayoutManager;
	class BlendModeManager;
}

// デバッグ用の3D線を描く（即時モード）
// ・どこからでも DrawLine などを呼ぶと、そのフレームだけ描かれる（毎フレーム呼び直す）
// ・頂点はワールド座標のまま UPLOAD ヒープの頂点バッファに直接書き、ViewProjection 1つで変換する
// ・深度テストあり（物の後ろは隠れる）と、なし（常に手前）の2種類を PSO を分けて描く
// ・エンジンは毎フレーム GPU の完了を待っているので、頂点バッファは1本で安全
//   （フレームを重ねて走らせるなら、フレーム数ぶんのリングにする）
class LineRenderer {
public:
	static constexpr uint32_t kMaxLines = 32768; // 深度あり・なしそれぞれの最大本数
	static constexpr uint32_t kMaxPreviewLines = 1024; // パーティクルエディタのプレビュー用の最大本数

	static LineRenderer* GetInstance() {
		static LineRenderer instance;
		return &instance;
	}

	// Renderer のコンストラクタから呼ぶ
	void Initialize(
		ID3D12Device* device,
		MyEngine::Rendering::PSOManager* psoManager,
		MyEngine::Rendering::ShaderManager* shaderManager,
		const MyEngine::Rendering::InputLayoutManager* inputLayoutManager,
		const MyEngine::Rendering::BlendModeManager* blendModeManager,
		ID3D12RootSignature* commonRootSignature
	);

	// Renderer のデストラクタから呼ぶ（デバイスより先にリソースを解放する）
	void Finalize();

	// ===== 描画要求（どこからでも呼べる） =====

	// 線分
	void DrawLine(const Vector3& a, const Vector3& b, const Vector4& color, bool depthTest = true);

	// XZ平面の円
	void DrawCircleXZ(const Vector3& center, float radius, const Vector4& color, int segments = 32, bool depthTest = true);

	// 矢印（先端の羽は線の長さ × headRatio）
	void DrawArrow(const Vector3& from, const Vector3& to, const Vector4& color, float headRatio = 0.2f, bool depthTest = true);

	// XZ平面のグリッド（center を中心に一辺 size、間隔 step）
	void DrawGridXZ(const Vector3& center, float size, float step, const Vector4& color, bool depthTest = true);

	// XYZ軸（X = 赤、Y = 緑、Z = 青）。行列や InputLayout の確認用
	void DrawAxis(const Vector3& origin, float length = 1.0f, bool depthTest = false);

	// ===== Renderer から呼ぶ =====

	// シーンのレンダーターゲットと深度バッファがセットされた状態で呼ぶ
	void Render(ID3D12GraphicsCommandList* cmdList, const Matrix4x4& viewProj);

	// 次のフレームのために線を空にする
	void Clear();

	// ===== パーティクルエディタのプレビュー用（シーンの線とは別の区画・別の CB） =====
	// ・シーンの線と混ざらないよう、頂点バッファの後ろにプレビュー専用の区画を持つ
	// ・ViewProjection もシーン用とは別の CB にする（1フレームに2回 Render すると上書きされるため）
	// ・SetEnabled(false) はシーンのデバッグ線用のスイッチなので、こちらには効かない

	// プレビュー用の線分
	void DrawPreviewLine(const Vector3& a, const Vector3& b, const Vector4& color);

	// プレビュー用の XZ 平面グリッド（center を中心に一辺 size、間隔 step）
	void DrawPreviewGridXZ(const Vector3& center, float size, float step, const Vector4& color);

	// プレビューのレンダーターゲットと深度バッファがセットされた状態で呼ぶ（描いたら空にする）
	void RenderPreview(ID3D12GraphicsCommandList* cmdList, const Matrix4x4& viewProj);

	// 表示のON/OFF（OFFの間は描画要求も無視する）
	void SetEnabled(bool enabled) { isEnabled_ = enabled; }
	bool IsEnabled() const { return isEnabled_; }

	// 今フレームの本数（デバッグ表示用）
	uint32_t GetLineCount() const { return depthCount_ + overlayCount_; }

private:
	LineRenderer() = default;
	~LineRenderer() = default;
	LineRenderer(const LineRenderer&) = delete;
	LineRenderer& operator=(const LineRenderer&) = delete;
	LineRenderer(LineRenderer&&) = delete;
	LineRenderer& operator=(LineRenderer&&) = delete;

private:
	// 頂点バッファ（前半 kMaxLines 本 = 深度あり、次の kMaxLines 本 = 常に手前、最後の kMaxPreviewLines 本 = プレビュー）
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
	LineVertex* mappedVertices_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vbView_{};

	// ViewProjection（b0 = 共通ルートシグネチャの Slot 0）
	ConstantBuffer<Matrix4x4> viewProjBuffer_;
	ConstantBuffer<Matrix4x4> previewViewProjBuffer_; // プレビュー用（シーン用とは別）

	// PSO（実体は PSOManager のキャッシュが持っている）
	ID3D12PipelineState* psoDepth_ = nullptr;
	ID3D12PipelineState* psoOverlay_ = nullptr;
	ID3D12RootSignature* rootSignature_ = nullptr;

	// 今フレームの本数
	uint32_t depthCount_ = 0;
	uint32_t overlayCount_ = 0;
	uint32_t previewCount_ = 0;

	bool isInitialized_ = false;
	bool isEnabled_ = true;
};
