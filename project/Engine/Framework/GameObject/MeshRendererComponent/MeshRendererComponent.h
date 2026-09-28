#pragma once
#include "Component.h"

namespace MyEngine::Rendering {
	class Model;
	class Material;

	enum class BlendModeType : uint8_t;
}

class MeshRendererComponent : public Component {
public:
	MeshRendererComponent();
	~MeshRendererComponent() override;

	void Initialize() override;
	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "Mesh Renderer"; }

	// モデル・テクスチャを変更する関数
	void SetModel(const std::string& modelPath);
	MyEngine::Rendering::Model* GetModel() const;

	MyEngine::Rendering::Material* GetMaterial() const;

	void SetTexture(const std::string& textureName);

	// マテリアルカラーを直接変更する関数
	void SetColor(const Vector4& color);

	// ライティングの有効・無効を切り替える関数
	void SetEnableLighting(bool flag);

	// ブレンドモードを設定する関数
	void SetBlendMode(MyEngine::Rendering::BlendModeType mode);

	// デプスの有効・無効を設定する関数
	void SetDepthEnable(bool flag);

	// レンダリングレイヤーを設定する関数
	void SetLayer(uint8_t layer);

	// 両面表示を設定する関数
	void SetDoubleSided(bool flag);

	// 表示・非表示を設定する関数
	void SetVisible(bool flag) { isVisible_ = flag; }
	bool IsVisible() const { return isVisible_; }

private:
	std::unique_ptr<MyEngine::Rendering::Model> model_ = nullptr;
	std::string modelPath_ = "Resources/AnimatedCube/AnimatedCube.gltf";	// デフォルトモデル
	std::string texPath_ = "white1x1";		// デフォルトテクスチャ
	uint32_t modelIndex_ = 0;
	uint32_t texIndex_ = 0;
	bool isDepthEnable_ = true;
	bool isVisible_ = true;
};