#pragma once
#include "RenderComponent.h"

namespace MyEngine::Rendering {
	class Renderer;
	class Model;
	class Material;
	class RenderCollector;
}

class NumberDrawerComponent : public MyEngine::Rendering::RenderComponent {
public:
	enum class Alignment {
		Left,
		Center,
		Right
	};

	NumberDrawerComponent();
	~NumberDrawerComponent() override;

	void Initialize() override;
	void Update() override;
	void ImGui() override;

	// 描画素材収集関数のオーバーライド
	void CollectRenderData(MyEngine::Rendering::RenderCollector* collector) const override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "Number Drawer"; }

	const std::vector<std::unique_ptr<MyEngine::Rendering::Model>>& GetDigitModels() const;

	MyEngine::Rendering::Material* GetMaterial();

	// 値の設定
	void SetValue(int value);
	int GetValue() const { return value_; }

	void SetTexture(const std::string& texPath);

	// 各種アクセッサ
	void SetPosition(const Vector2& pos) { position_ = pos; }
	const Vector2& GetPosition() const { return position_; }

	void SetSize(const Vector2& size) { size_ = size; }
	const Vector2& GetSize() const { return size_; }

	void SetSpacing(float spacing) { spacing_ = spacing; }
	float GetSpacing() const { return spacing_; }

	void SetScale(const Vector2& scale) { scale_ = scale; }
	const Vector2& GetScale() const { return scale_; }

	void SetColor(const Vector4& color) { color_ = color; }
	const Vector4& GetColor() const { return color_; }

	void SetAnchorPoint(const Vector2& anchor) { anchorPoint_ = anchor; }
	const Vector2& GetAnchorPoint() const { return anchorPoint_; }

	void SetLayer(uint8_t layer) { layer_ = layer; }
	uint8_t GetLayer() const { return layer_; }

	void SetAlignment(Alignment align) { alignment_ = align; }
	Alignment GetAlignment() const { return alignment_; }

private:
	void UpdateModels();

private:
	std::vector<std::unique_ptr<MyEngine::Rendering::Model>> digitModels_;
	std::string texPath_ = "Resources/number_font.png"; // 生成したアトラス画像
	std::string modelPath_ = "Resources/plane/plane.obj";

	int value_ = 0;                     // 表示する値
	Vector2 position_ = { 100.0f, 100.0f };
	Vector2 size_ = { 32.0f, 64.0f };    // 1文字のサイズ
	float spacing_ = 0.0f;               // 文字間の隙間（ピクセル）
	Vector2 scale_ = { 1.0f, 1.0f };
	Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	Vector2 anchorPoint_ = { 0.5f, 0.5f };
	uint8_t layer_ = 4;
	Alignment alignment_ = Alignment::Center;

	uint32_t modelIndex_ = 0;
	uint32_t texIndex_ = 0;
};
