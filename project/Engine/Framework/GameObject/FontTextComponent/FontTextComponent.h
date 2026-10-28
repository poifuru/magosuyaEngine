#pragma once
#include "RenderComponent.h"
#include "MeshData.h"
#include "ConstantBuffer.h"
#include "TransformMatrixData.h"

class Font;

// SDF テキストの縁取り・影の設定（TextSDF.hlsli の TextEffect と同じ並び）
struct TextEffectData {
	Vector4 outlineColor;
	Vector4 shadowColor;
	float outlineWidth;     // SDF の値の単位
	float shadowSoftness;   // SDF の値の単位
	float padding[2];
};

namespace MyEngine::Rendering {
	class Material;
	class RenderCollector;
}

// エンジンに入れたフォント(TTF/OTF)で、画面に自由なテキストを表示するコンポーネント
// 全文字の四角形を1つの頂点バッファにまとめて、1回の描画で出す
class FontTextComponent : public MyEngine::Rendering::RenderComponent {
public:
	// 行の揃え方
	enum class Alignment {
		Left,
		Center,
		Right
	};

	FontTextComponent();
	~FontTextComponent() override = default;

	void Initialize() override;
	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "Font Text"; }

	// 描画素材収集関数のオーバーライド
	void CollectRenderData(MyEngine::Rendering::RenderCollector* collector) const override;

	// テキスト (UTF-8。"\n" で改行)
	void SetText(const std::string& text);
	const std::string& GetText() const { return text_; }

	// フォント（ファイルと焼くサイズ）
	void SetFont(const std::string& fontPath, float bakeSize);
	const std::string& GetFontPath() const { return fontPath_; }

	// 各種パラメータへのアクセッサ
	void SetPosition(const Vector2& pos) { position_ = pos; }
	const Vector2& GetPosition() const { return position_; }

	// 表示する文字の高さ px（焼いたサイズと違うと拡大縮小される）
	void SetFontSize(float size) { fontSize_ = size; }
	float GetFontSize() const { return fontSize_; }

	void SetRotation(float rot) { rotation_ = rot; }
	float GetRotation() const { return rotation_; }

	void SetColor(const Vector4& color) { color_ = color; }
	const Vector4& GetColor() const { return color_; }

	// テキスト全体の箱のどこを position に合わせるか (0,0 で左上、0.5,0.5 で中心)
	void SetAnchorPoint(const Vector2& anchor) { anchorPoint_ = anchor; }
	const Vector2& GetAnchorPoint() const { return anchorPoint_; }

	void SetAlignment(Alignment align) { alignment_ = align; isDirty_ = true; }
	Alignment GetAlignment() const { return alignment_; }

	// 文字間の追加の隙間 px（焼いたサイズ基準）
	void SetLetterSpacing(float spacing) { letterSpacing_ = spacing; isDirty_ = true; }
	// 行間の倍率 (1.0 でフォント標準)
	void SetLineSpacing(float scale) { lineSpacing_ = scale; isDirty_ = true; }

	void SetLayer(uint8_t layer) { layer_ = layer; }
	uint8_t GetLayer() const { return layer_; }

	// テキスト全体の大きさ px（表示サイズ基準）
	Vector2 GetTextSize() const;

	// --- SDF ---
	// SDF で描くか（拡大しても滲まない。縁取り・影は SDF の時だけ使える）
	void SetUseSDF(bool useSDF);
	bool GetUseSDF() const { return useSDF_; }

	// 縁取り（太さは表示サイズ基準の px。0 で無し）
	void SetOutline(const Vector4& color, float width) { outlineColor_ = color; outlineWidth_ = width; }

	// 影（オフセットは表示サイズ基準の px）
	void SetShadowEnabled(bool enabled) { shadowEnabled_ = enabled; isDirty_ = true; }
	void SetShadow(const Vector4& color, const Vector2& offset, float softness) {
		shadowColor_ = color; shadowOffset_ = offset; shadowSoftness_ = softness;
	}

private:
	// フォントを取り直してマテリアルに反映する
	void ApplyFont();
	// 文字列から四角形を並べ直して頂点バッファに書き込む
	void RebuildMesh();

private:
	// 1つのコンポーネントで出せる最大文字数
	static constexpr uint32_t kMaxChars = 512;
	// 四角形の最大数（影を付けると1文字につき2枚使う）
	static constexpr uint32_t kMaxQuads = kMaxChars * 2;

	// 描画用
	MyEngine::Rendering::Mesh mesh_;
	VertexBuffer<VertexData> vertexBuffer_;
	IndexBuffer<uint32_t> indexBuffer_;
	std::shared_ptr<MyEngine::Rendering::Material> material_ = nullptr;
	ConstantBuffer<TransformMatrixData> transformBuffer_;
	ConstantBuffer<TextEffectData> effectBuffer_;

	Font* font_ = nullptr;
	bool isDirty_ = true;          // 文字の並びを作り直す必要があるか
	Vector2 boxSize_ = { 0.0f, 0.0f }; // テキスト全体の箱の大きさ（焼いたサイズ基準 px）
	Vector2 builtShadowOffset_ = { 0.0f, 0.0f }; // 今の頂点を作った時の影のずれ（焼いたサイズ基準 px）

	// 設定値
	std::string fontPath_ = "Resources/AppliMincho/HigureGothic-Regular.ttf";
	float bakeSize_ = 32.0f;       // アトラスに焼く文字の高さ px
	std::string text_ = "Hello, テキスト!";

	Vector2 position_ = { 640.0f, 360.0f }; // 画面座標 (px)
	float fontSize_ = 48.0f;                // 表示する文字の高さ px
	float rotation_ = 0.0f;                 // 角度 (ラジアン)
	Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	Vector2 anchorPoint_ = { 0.5f, 0.5f };
	Alignment alignment_ = Alignment::Left;
	float letterSpacing_ = 0.0f;
	float lineSpacing_ = 1.0f;
	uint8_t layer_ = 4;                     // UIレイヤー

	// SDF・縁取り・影
	bool useSDF_ = true;
	Vector4 outlineColor_ = { 0.0f, 0.0f, 0.0f, 1.0f };
	float outlineWidth_ = 0.0f;             // px
	bool shadowEnabled_ = false;
	Vector4 shadowColor_ = { 0.0f, 0.0f, 0.0f, 0.6f };
	Vector2 shadowOffset_ = { 3.0f, 3.0f }; // px
	float shadowSoftness_ = 2.0f;           // px
};
