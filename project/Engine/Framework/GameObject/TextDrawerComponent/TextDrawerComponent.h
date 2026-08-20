#pragma once
#include "Component.h"

namespace MyEngine::Rendering {
	class Renderer;
	class Model;
	class Material;
}

class TextDrawerComponent : public Component {
public:
	enum class Alignment {
		Left,
		Center,
		Right
	};

	TextDrawerComponent();
	~TextDrawerComponent() override;

	void Initialize() override;
	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "Text Drawer"; }

	const std::vector<std::unique_ptr<MyEngine::Rendering::Model>>& GetCharacterModels() const;

	MyEngine::Rendering::Material* GetMaterial();

	// テキストの設定
	void SetText(const std::string& text);
	const std::string& GetText() const { return text_; }

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
	std::vector<std::unique_ptr<MyEngine::Rendering::Model>> charModels_;
	std::string text_ = "";
	std::string texPath_ = "Resources/number_font.png";
	std::string modelPath_ = "Resources/plane/plane.obj";

	Vector2 position_ = { 640.0f, 200.0f };
	Vector2 size_ = { 32.0f, 64.0f };    // 1文字のサイズ
	float spacing_ = 0.0f;               // 文字間の隙間
	Vector2 scale_ = { 1.0f, 1.0f };
	Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	Vector2 anchorPoint_ = { 0.5f, 0.5f };
	uint8_t layer_ = 4;
	Alignment alignment_ = Alignment::Center;

	uint32_t modelIndex_ = 0;
	uint32_t texIndex_ = 0;
};
