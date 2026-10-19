#include "PCH.h"
#include "SpriteComponent.h"
#include "GameObject.h"
#include "BaseScene.h"
#include "TextureManager.h"
#include "WindowsAPI.h"
#include "MathFunction.h"
#include "Material.h"
#include "GraphicsDevice.h"
#include "RenderCollector.h"

SpriteComponent::SpriteComponent() = default;

void SpriteComponent::Initialize() {
	if (isInitialized_) return;
	isInitialized_ = true;

	GameObject* owner = GetGameObject();
	if (!owner) return;
	SceneContext* context = owner->GetContext();
	if (!context || !context->graphicsDevice) return;

	ID3D12Device* device = context->graphicsDevice->GetDevice();

	// 1. コード上で直接矩形（Quad）メッシュを生成（ファイル読み込み不要）
	// (0, 0) 〜 (1, 1) の 4 頂点
	std::vector<VertexData> vertices = {
		// 左上
		{ { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
		// 右上
		{ { 1.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
		// 左下
		{ { 0.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } },
		// 右下
		{ { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } },
	};
	// 2枚の三角形インデックス
	std::vector<uint32_t> indices = { 0, 1, 2, 1, 3, 2 };

	mesh_.Initialize(device, vertices, indices);

	// 2. マテリアル & トランスフォームバッファの初期化
	material_ = std::make_shared<MyEngine::Rendering::Material>();
	material_->Initialize(context->graphicsDevice, context->heapManager);
	material_->SetDepthEnable(false); // 2Dなのでデプス無効
	material_->SetDepthWrite(false);
	material_->SetBlendMode(MyEngine::Rendering::BlendModeType::Alpha); // 半透明
	material_->SetLayer(layer_);
	material_->SetDoubleSided(true);
	material_->SetColor(color_);
	material_->SetEnableLighting(FALSE);

	transformBuffer_.Initialize(device);

	// デフォルトテクスチャのロードと適用
	SetTexture(texPath_);
}

void SpriteComponent::Update() {
	if (!material_ || !gameObject_) return;

	// 画面サイズ取得（1280 x 720）
	float screenWidth = static_cast<float>(WindowsAPI::GetInstance()->GetWindowWidth());
	float screenHeight = static_cast<float>(WindowsAPI::GetInstance()->GetWindowHeight());
	if (screenWidth <= 0.0f) screenWidth = 1280.0f;
	if (screenHeight <= 0.0f) screenHeight = 720.0f;

	// 自然な2Dアフィン変換行列の計算
	// 1. アンカーポイントのオフセット
	Matrix4x4 matAnchor = Math::MakeTranslateMatrix({ -anchorPoint_.x, -anchorPoint_.y, 0.0f });

	// 2. スケール（ピクセルサイズそのもの）
	Matrix4x4 matScale = Math::MakeScaleMatrix({ size_.x * scale_.x, size_.y * scale_.y, 1.0f });

	// 3. 回転 (Z軸)
	float totalRot = rotation_ + gameObject_->GetTransform().rotate.z;
	Matrix4x4 matRot = Math::MakeRotateZMatrix(totalRot);

	// 4. 画面上の配置座標 (px)
	Vector3 finalPos = {
		position_.x + gameObject_->GetTransform().translate.x,
		position_.y + gameObject_->GetTransform().translate.y,
		0.0f
	};
	Matrix4x4 matTrans = Math::MakeTranslateMatrix(finalPos);

	// ワールド行列の合成: Anchor -> Scale -> Rot -> Translate
	Matrix4x4 world = Math::Multiply(matAnchor, matScale);
	world = Math::Multiply(world, matRot);
	world = Math::Multiply(world, matTrans);

	// 平行投影行列 (左上が (0,0)、右下が (screenWidth, screenHeight))
	Matrix4x4 projection = Math::MakeOrthographicMatrix(0.0f, 0.0f, screenWidth, screenHeight, 0.0f, 100.0f);

	// バッファ更新
	TransformMatrixData transformData{};
	transformData.World = world;
	transformData.WVP = Math::Multiply(world, projection);
	transformData.WorldInverseTranspose = Math::MakeIdentity4x4();
	transformBuffer_.Update(transformData);

	// マテリアル更新
	material_->SetColor(color_);
	material_->SetLayer(layer_);
	material_->Update();
}

void SpriteComponent::CollectRenderData(MyEngine::Rendering::RenderCollector* collector) const {
	if (!isVisible_ || !material_ || !collector) return;

	MyEngine::Rendering::RenderItem item{};
	item.mesh = &mesh_;
	item.material = material_.get();
	item.transformAddr = transformBuffer_.GetGPUVirtualAddress();
	item.customAddr = 0;
	item.debugName = gameObject_ ? gameObject_->GetName().c_str() : "Sprite";

	collector->Submit(item);
}

void SpriteComponent::SetTexture(const std::string& textureName) {
	texPath_ = textureName;
	if (gameObject_ && gameObject_->GetContext() && gameObject_->GetContext()->textureManager) {
		texIndex_ = gameObject_->GetContext()->textureManager->LoadTexture(texPath_);
		if (material_) {
			material_->SetTextureIndex(texIndex_);
		}
	}
}

void SpriteComponent::SetColor(const Vector4& color) {
	color_ = color;
	if (material_) {
		material_->SetColor(color_);
	}
}

void SpriteComponent::ImGui() {
#ifdef USEIMGUI
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
	j["isVisible"] = isVisible_;
}

void SpriteComponent::Deserialize(const json& j) {
	if (j.contains("texPath")) texPath_ = j["texPath"];
	if (j.contains("position")) position_ = { j["position"][0], j["position"][1] };
	if (j.contains("size")) size_ = { j["size"][0], j["size"][1] };
	if (j.contains("rotation")) rotation_ = j["rotation"];
	if (j.contains("scale")) scale_ = { j["scale"][0], j["scale"][1] };
	if (j.contains("color")) color_ = { j["color"][0], j["color"][1], j["color"][2], j["color"][3] };
	if (j.contains("anchorPoint")) anchorPoint_ = { j["anchorPoint"][0], j["anchorPoint"][1] };
	if (j.contains("layer")) layer_ = j["layer"];
	if (j.contains("isVisible")) isVisible_ = j["isVisible"];

	Initialize();
	SetTexture(texPath_);
}
