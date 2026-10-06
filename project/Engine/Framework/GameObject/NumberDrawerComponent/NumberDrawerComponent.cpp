#include "PCH.h"
#include "NumberDrawerComponent.h"
#include "GameObject.h"
#include "BaseScene.h"
#include "ModelFactory.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "WindowsAPI.h"
#include "MathFunction.h"
#include "BaseCamera.h"
#include "Model.h"

NumberDrawerComponent::NumberDrawerComponent() = default;

NumberDrawerComponent::~NumberDrawerComponent() = default;

void NumberDrawerComponent::Initialize() {
	if (isInitialized_) return;
	isInitialized_ = true;

	GameObject* owner = GetGameObject();
	if (!owner) return;
	SceneContext* context = owner->GetContext();
	if (!context) return;

	// plane.objのインデックス取得
	modelIndex_ = context->modelManager->LoadModelData(modelPath_);

	// テクスチャのロード
	texIndex_ = context->textureManager->LoadTexture(texPath_);

	// 初期表示モデル数の生成
	UpdateModels();
}

void NumberDrawerComponent::Update() {
	if (!gameObject_) return;

	UpdateModels();
}

void NumberDrawerComponent::ImGui() {
#ifdef USEIMGUI
	// テクスチャ
	char texBuf[256];
	strcpy_s(texBuf, texPath_.c_str());
	if (ImGui::InputText("Texture Atlas", texBuf, sizeof(texBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
		SetTexture(texBuf);
	}

	int val = value_;
	if (ImGui::DragInt("Value", &val, 1)) {
		SetValue(val);
	}

	ImGui::DragFloat2("Position", &position_.x, 1.0f);
	ImGui::DragFloat2("Char Size", &size_.x, 1.0f, 0.0f, 4096.0f);
	ImGui::DragFloat("Spacing", &spacing_, 1.0f, -100.0f, 100.0f);
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

	const char* items[] = { "Left", "Center", "Right" };
	int currentAlign = static_cast<int>(alignment_);
	if (ImGui::Combo("Alignment", &currentAlign, items, IM_ARRAYSIZE(items))) {
		alignment_ = static_cast<Alignment>(currentAlign);
	}
#endif
}

void NumberDrawerComponent::Serialize(json& j) const {
	j["type"] = "NumberDrawerComponent";
	j["texPath"] = texPath_;
	j["value"] = value_;
	j["position"] = { position_.x, position_.y };
	j["size"] = { size_.x, size_.y };
	j["spacing"] = spacing_;
	j["scale"] = { scale_.x, scale_.y };
	j["color"] = { color_.x, color_.y, color_.z, color_.w };
	j["anchorPoint"] = { anchorPoint_.x, anchorPoint_.y };
	j["layer"] = layer_;
	j["alignment"] = static_cast<int>(alignment_);
}

void NumberDrawerComponent::Deserialize(const json& j) {
	isInitialized_ = true;
	if (j.contains("texPath")) texPath_ = j["texPath"];
	if (j.contains("value")) value_ = j["value"];
	if (j.contains("position")) position_ = { j["position"][0], j["position"][1] };
	if (j.contains("size")) size_ = { j["size"][0], j["size"][1] };
	if (j.contains("spacing")) spacing_ = j["spacing"];
	if (j.contains("scale")) scale_ = { j["scale"][0], j["scale"][1] };
	if (j.contains("color")) color_ = { j["color"][0], j["color"][1], j["color"][2], j["color"][3] };
	if (j.contains("anchorPoint")) anchorPoint_ = { j["anchorPoint"][0], j["anchorPoint"][1] };
	if (j.contains("layer")) layer_ = j["layer"];
	if (j.contains("alignment")) alignment_ = static_cast<Alignment>(j["alignment"]);

	GameObject* owner = GetGameObject();
	if (owner && owner->GetContext()) {
		SceneContext* context = owner->GetContext();
		modelIndex_ = context->modelManager->LoadModelData(modelPath_);
		SetTexture(texPath_);
	}
}

const std::vector<std::unique_ptr<MyEngine::Rendering::Model>>& NumberDrawerComponent::GetDigitModels() const {
	return digitModels_;
}

void NumberDrawerComponent::SetValue(int value) {
	value_ = value;
}

void NumberDrawerComponent::SetTexture(const std::string& texPath) {
	GameObject* owner = GetGameObject();
	if (!owner) return;
	SceneContext* context = owner->GetContext();
	if (!context) return;

	texPath_ = texPath;
	texIndex_ = context->textureManager->LoadTexture(texPath_);
}

void NumberDrawerComponent::UpdateModels() {
	GameObject* owner = GetGameObject();
	if (!owner) return;
	SceneContext* context = owner->GetContext();
	if (!context) return;

	std::string valStr = std::to_string(value_);
	size_t len = valStr.length();

	// 必要桁数に合わせてモデル数をリサイズ
	if (digitModels_.size() != len) {
		digitModels_.clear();
		for (size_t i = 0; i < len; ++i) {
			auto model = context->modelFactory->CreateModel(modelIndex_, 0);
			if (model) {
				model->SetDepthEnable(false);
				model->SetBlendMode(MyEngine::Rendering::BlendModeType::Alpha);
				model->SetDoubleSided(true);
				model->SetLayer(layer_);
				if (auto mat = model->GetMaterial()) {
					mat->SetEnableLighting(false);
				}
			}
			digitModels_.push_back(std::move(model));
		}
	}

	// 画面サイズ（レンダーターゲット基準: 1280x720）
	float screenWidth = 1280.0f;
	float screenHeight = 720.0f;
	Matrix4x4 projection = Math::MakeOrthographicMatrix(0.0f, 0.0f, screenWidth, screenHeight, -1.0f, 100.0f);

	// 全体の幅の計算
	float charW = size_.x * scale_.x;
	float charH = size_.y * scale_.y;
	float totalWidth = charW * len + spacing_ * (len - 1);

	// アライメントによる並び始めXオフセットの計算
	float startX = 0.0f;
	if (alignment_ == Alignment::Left) {
		startX = 0.0f;
	} else if (alignment_ == Alignment::Center) {
		startX = -totalWidth / 2.0f + charW / 2.0f;
	} else if (alignment_ == Alignment::Right) {
		startX = -totalWidth + charW / 2.0f;
	}

	// アンカーポイントによるオフセット
	float anchorOffsetX = totalWidth * (0.5f - anchorPoint_.x);
	float anchorOffsetY = charH * (0.5f - anchorPoint_.y);

	// 基準座標
	float baseX = position_.x + gameObject_->GetTransform().translate.x + anchorOffsetX;
	float baseY = position_.y + gameObject_->GetTransform().translate.y + anchorOffsetY;

	// 各桁の更新
	for (size_t i = 0; i < len; ++i) {
		auto& model = digitModels_[i];
		if (!model) continue;

		char c = valStr[i];
		int digit = (c >= '0' && c <= '9') ? (c - '0') : 0;

		// マテリアルへのUV座標の割り当て
		if (auto mat = model->GetMaterial()) {
			mat->SetTextureIndex(texIndex_);
			mat->SetColor(color_);
			// UVのU（左右）のみを反転させて裏返りによる鏡像を補正し、V（上下）はデフォルトのままにする
			mat->SetUvScale({ -0.1f, 1.0f, 1.0f });
			mat->SetUvTranslate({ 0.1f * digit + 0.1f, 0.0f, 0.0f });
		}

		// 1文字ごとのトランスフォーム設定
		Vector3 finalScale = { charW / 2.0f, -charH / 2.0f, 1.0f }; // Yをマイナスにして裏返りを相殺し鏡像を防ぐ
		Vector3 finalRotate = { 0.0f, 0.0f, gameObject_->GetTransform().rotate.z };
		Vector3 finalTranslate = {
			baseX + startX + static_cast<float>(i) * (charW + spacing_),
			baseY,
			0.0f
		};

		model->SetScale(finalScale);
		model->SetRotate(finalRotate);
		model->SetTranslate(finalTranslate);
		model->SetLayer(layer_);

		// 深度テスト無効、ブレンドモード半透明、ライティング無効、両面表示
		model->SetDepthEnable(false);
		model->SetBlendMode(MyEngine::Rendering::BlendModeType::Alpha);
		model->SetDoubleSided(true);
		if (auto mat = model->GetMaterial()) {
			mat->SetEnableLighting(false);
		}

		CameraData dummyCamera{};
		dummyCamera.vp = projection;
		model->Update(&dummyCamera);
	}
}
