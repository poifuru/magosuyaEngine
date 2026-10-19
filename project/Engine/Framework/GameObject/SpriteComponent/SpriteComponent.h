#pragma once
#include "RenderComponent.h"
#include "MeshData.h"
#include "ConstantBuffer.h"
#include "TransformMatrixData.h"

namespace MyEngine::Rendering {
	class Material;
	class RenderCollector;
}

class SpriteComponent : public MyEngine::Rendering::RenderComponent {
public:
	SpriteComponent();
	~SpriteComponent() override = default;

	void Initialize() override;
	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "Sprite Component"; }

	// 描画素材収集関数のオーバーライド
	void CollectRenderData(MyEngine::Rendering::RenderCollector* collector) const override;

	MyEngine::Rendering::Material* GetMaterial() const { return material_.get(); }

	// テクスチャ設定
	void SetTexture(const std::string& textureName);

	// 各種パラメータへのアクセッサ
	void SetPosition(const Vector2& pos) { position_ = pos; }
	const Vector2& GetPosition() const { return position_; }

	void SetSize(const Vector2& size) { size_ = size; }
	const Vector2& GetSize() const { return size_; }

	void SetRotation(float rot) { rotation_ = rot; }
	float GetRotation() const { return rotation_; }

	void SetScale(const Vector2& scale) { scale_ = scale; }
	const Vector2& GetScale() const { return scale_; }

	void SetColor(const Vector4& color);
	const Vector4& GetColor() const { return color_; }

	void SetAnchorPoint(const Vector2& anchor) { anchorPoint_ = anchor; }
	const Vector2& GetAnchorPoint() const { return anchorPoint_; }

	void SetLayer(uint8_t layer) { layer_ = layer; }
	uint8_t GetLayer() const { return layer_; }

private:
	// 3D Model の代わりに自前で Quad メッシュ、マテリアル、バッファを保持
	MyEngine::Rendering::StaticMesh mesh_;
	std::shared_ptr<MyEngine::Rendering::Material> material_ = nullptr;
	ConstantBuffer<TransformMatrixData> transformBuffer_;

	std::string texPath_ = "Resources/uvChecker.png"; // デフォルトテクスチャ
	uint32_t texIndex_ = 0;

	Vector2 position_ = { 100.0f, 100.0f }; // 画面座標 (px)
	Vector2 size_ = { 100.0f, 100.0f };     // サイズ (px)
	float rotation_ = 0.0f;                 // 角度 (ラジアン)
	Vector2 scale_ = { 1.0f, 1.0f };        // スケール倍率
	Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	Vector2 anchorPoint_ = { 0.5f, 0.5f };  // アンカーポイント (0.5, 0.5 で中心)
	uint8_t layer_ = 4;                     // UIレイヤー
};
