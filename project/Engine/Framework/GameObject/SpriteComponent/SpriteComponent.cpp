#include "PCH.h"
#include "SpriteComponent.h"
#include "GameObject.h"
#include "BaseScene.h"
#include "ModelFactory.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "WindowsAPI.h"
#include "MathFunction.h"
#include "BaseCamera.h"
#include "Model.h"
#include "Material.h"
#include "RenderingModel.h"

SpriteComponent::~SpriteComponent() = default;

SpriteComponent::SpriteComponent() = default;

void SpriteComponent::Initialize() {
	if (isInitialized_) return;
	isInitialized_ = true;

	GameObject* owner = GetGameObject();
	if (!owner) return;
	SceneContext* context = owner->GetContext();
	if (!context) return;

	// デフォルトモデルのロードと作成
	modelIndex_ = context->modelManager->LoadModelData(modelPath_);
	model_.reset();
	model_ = context->modelFactory->CreateModel(modelIndex_, 0);

	// デフォルトテクスチャのロードと適用
	SetTexture(texPath_);

	if (model_) {
		model_->SetDepthEnable(false); // デプス無効
		model_->SetBlendMode(MyEngine::Rendering::BlendModeType::Alpha); // 半透明
		model_->SetLayer(layer_);
		model_->SetDoubleSided(true); // 両面表示を有効化
		if (auto mat = model_->GetMaterial()) {
			mat->SetEnableLighting(false); // ライティング無効化
		}
	}
}

void SpriteComponent::Update() {
	if (!model_ || !gameObject_) return;

	// パラメータをモデルに適用
	model_->SetDepthEnable(false);
	model_->SetBlendMode(MyEngine::Rendering::BlendModeType::Alpha);
	model_->SetLayer(layer_);
	if (auto mat = model_->GetMaterial()) {
		mat->SetColor(color_);
		// UVのU（左右）のみを反転させて裏返りによる鏡像を補正し、V（上下）はデフォルトのままにする
		mat->SetUvScale({ -1.0f, 1.0f, 1.0f });
		mat->SetUvTranslate({ 1.0f, 0.0f, 0.0f });
	}

	// 画面サイズ
	float screenWidth = 1280.0f;
	float screenHeight = 720.0f;

	// 平行投影行列の計算 (Nearを-1.0fに拡張してZ=0.0fのクリップを防止)
	Matrix4x4 projection = Math::MakeOrthographicMatrix(0.0f, 0.0f, screenWidth, screenHeight, -1.0f, 100.0f);

	// plane.objは -1.0〜1.0 (サイズ 2.0x2.0) なので、
	// 希望のサイズにするには width / 2.0f, height / 2.0f をスケールにかける。
	// Y下向きをプラスにするため、Yスケールをマイナスにする。（UV反転と組み合わせて鏡像を防止）
	Vector3 finalScale = {
		(size_.x * scale_.x) / 2.0f,
		-(size_.y * scale_.y) / 2.0f,
		1.0f
	};

	// 回転の計算 (Z軸回転のみ)
	Vector3 finalRotate = { 0.0f, 0.0f, rotation_ + gameObject_->GetTransform().rotate.z };

	// アンカーポイントを考慮した平行移動座標
	// (0.5, 0.5) を中心とし、そこからのズレとGameObjectのTransform.translateを加算
	Vector3 finalTranslate = {
		position_.x + gameObject_->GetTransform().translate.x + size_.x * scale_.x * (0.5f - anchorPoint_.x),
		position_.y + gameObject_->GetTransform().translate.y + size_.y * scale_.y * (0.5f - anchorPoint_.y),
		0.0f
	};

	// model_のTransformを同期させる
	model_->SetScale(finalScale);
	model_->SetRotate(finalRotate);
	model_->SetTranslate(finalTranslate);

	// dummyCamera の設定 (ビュー・プロジェクションに OrthographicMatrix を設定)
	CameraData dummyCamera{};
	dummyCamera.vp = projection;

	// model_->Updateを実行。これでWVPバッファへの転送が安全に行われる。
	model_->Update(&dummyCamera);
}

void SpriteComponent::ImGui() {
#ifdef USEIMGUI
	// テクスチャ
	char texBuf[256];
	strcpy_s(texBuf, texPath_.c_str());
	if (ImGui::InputText("Texture", texBuf, sizeof(texBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
		SetTexture(texBuf);
	}

	ImGui::DragFloat2("Position", &position_.x, 1.0f);
	ImGui::DragFloat2("Size", &size_.x, 1.0f, 0.0f, 4096.0f);
	ImGui::DragFloat("Rotation", &rotation_, 0.01f);
	ImGui::DragFloat2("Scale", &scale_.x, 0.01f);

	float colorArr[4] = { color_.x, color_.y, color_.z, color_.w };
	if (ImGui::ColorEdit4("Color", colorArr)) {
		color_ = { colorArr[0], colorArr[1], colorArr[2], colorArr[3] };
	}

	ImGui::DragFloat2("AnchorPoint", &anchorPoint_.x, 0.05f, 0.0f, 1.0f);

	int layerVal = layer_;
	if (ImGui::DragInt("Layer", &layerVal, 1.0f, 0, 255)) {
		layer_ = static_cast<uint8_t>(layerVal);
	}
#endif
}

void SpriteComponent::Serialize(json& j) const {
	j["type"] = "SpriteComponent";
	j["texPath"] = texPath_;
	j["position"] = { position_.x, position_.y };
	j["size"] = { size_.x, size_.y };
	j["rotation"] = rotation_;
	j["scale"] = { scale_.x, scale_.y };
	j["color"] = { color_.x, color_.y, color_.z, color_.w };
	j["anchorPoint"] = { anchorPoint_.x, anchorPoint_.y };
	j["layer"] = layer_;
}

void SpriteComponent::Deserialize(const json& j) {
	isInitialized_ = true;
	if (j.contains("texPath")) texPath_ = j["texPath"];
	if (j.contains("position")) position_ = { j["position"][0], j["position"][1] };
	if (j.contains("size")) size_ = { j["size"][0], j["size"][1] };
	if (j.contains("rotation")) rotation_ = j["rotation"];
	if (j.contains("scale")) scale_ = { j["scale"][0], j["scale"][1] };
	if (j.contains("color")) color_ = { j["color"][0], j["color"][1], j["color"][2], j["color"][3] };
	if (j.contains("anchorPoint")) anchorPoint_ = { j["anchorPoint"][0], j["anchorPoint"][1] };
	if (j.contains("layer")) layer_ = j["layer"];

	// モデルとテクスチャの再生成・再ロード
	GameObject* owner = GetGameObject();
	if (owner && owner->GetContext()) {
		SceneContext* context = owner->GetContext();
		modelIndex_ = context->modelManager->LoadModelData(modelPath_);
		model_.reset();
		model_ = context->modelFactory->CreateModel(modelIndex_, 0);

		SetTexture(texPath_);

		if (model_) {
			model_->SetDepthEnable(false);
			model_->SetBlendMode(MyEngine::Rendering::BlendModeType::Alpha);
			model_->SetLayer(layer_);
			model_->SetDoubleSided(true); // 両面表示を有効化
			if (auto mat = model_->GetMaterial()) {
				mat->SetEnableLighting(false); // ライティング無効化
			}
		}
	}
}

MyEngine::Rendering::Model* SpriteComponent::GetModel() const {
	return model_.get();
}

MyEngine::Rendering::Material* SpriteComponent::GetMaterial() const {
	return model_->GetMaterial();
}

void SpriteComponent::SetTexture(const std::string& textureName) {
	GameObject* owner = GetGameObject();
	if (!owner) return;
	SceneContext* context = owner->GetContext();
	if (!context) return;

	texPath_ = textureName;

	if (model_) {
		texIndex_ = context->textureManager->LoadTexture(texPath_);
		model_->SetTextureIndex(texIndex_);
	}
}

void SpriteComponent::SetColor(const Vector4& color) {
	color_ = color;
	if (model_) {
		if (auto mat = model_->GetMaterial()) {
			mat->SetColor(color);
		}
	}
}
