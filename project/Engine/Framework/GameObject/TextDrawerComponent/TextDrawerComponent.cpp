#include "PCH.h"
#include "TextDrawerComponent.h"
#include "GameObject.h"
#include "BaseScene.h"
#include "ModelFactory.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "WindowsAPI.h"
#include "MathFunction.h"
#include "BaseCamera.h"
#include "Model.h"

TextDrawerComponent::TextDrawerComponent() = default;

TextDrawerComponent::~TextDrawerComponent() = default;

void TextDrawerComponent::Initialize() {
	if (isInitialized_) return;
	isInitialized_ = true;

	GameObject* owner = GetGameObject();
	if (!owner) return;
	SceneContext* context = owner->GetContext();
	if (!context) return;

	modelIndex_ = context->modelManager->LoadModelData(modelPath_);
	texIndex_ = context->textureManager->LoadTexture(texPath_);

	UpdateModels();
}

void TextDrawerComponent::Update() {
	if (!gameObject_) return;

	UpdateModels();
}

void TextDrawerComponent::ImGui() {
#ifdef USEIMGUI
	char texBuf[256];
	strcpy_s(texBuf, texPath_.c_str());
	if (ImGui::InputText("Texture Atlas", texBuf, sizeof(texBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
		SetTexture(texBuf);
	}

	char textBuf[512];
	strcpy_s(textBuf, text_.c_str());
	if (ImGui::InputText("Text", textBuf, sizeof(textBuf))) {
		SetText(textBuf);
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

void TextDrawerComponent::Serialize(json& j) const {
	j["type"] = "TextDrawerComponent";
	j["texPath"] = texPath_;
	j["text"] = text_;
	j["position"] = { position_.x, position_.y };
	j["size"] = { size_.x, size_.y };
	j["spacing"] = spacing_;
	j["scale"] = { scale_.x, scale_.y };
	j["color"] = { color_.x, color_.y, color_.z, color_.w };
	j["anchorPoint"] = { anchorPoint_.x, anchorPoint_.y };
	j["layer"] = layer_;
	j["alignment"] = static_cast<int>(alignment_);
}

void TextDrawerComponent::Deserialize(const json& j) {
	isInitialized_ = true;
	if (j.contains("texPath")) texPath_ = j["texPath"];
	if (j.contains("text")) text_ = j["text"];
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

const std::vector<std::unique_ptr<MyEngine::Rendering::Model>>& TextDrawerComponent::GetCharacterModels() const {
	return charModels_;
}

MyEngine::Rendering::Material* TextDrawerComponent::GetMaterial() {
	if (!charModels_.empty() && charModels_[0]) {
		return charModels_[0]->GetMaterial();
	}
	return nullptr;
}

void TextDrawerComponent::SetText(const std::string& text) {
	text_ = text;
}

void TextDrawerComponent::SetTexture(const std::string& texPath) {
	GameObject* owner = GetGameObject();
	if (!owner) return;
	SceneContext* context = owner->GetContext();
	if (!context) return;

	texPath_ = texPath;
	texIndex_ = context->textureManager->LoadTexture(texPath_);
}

void TextDrawerComponent::UpdateModels() {
	GameObject* owner = GetGameObject();
	if (!owner) return;
	SceneContext* context = owner->GetContext();
	if (!context) return;

	size_t len = text_.length();

	if (charModels_.size() != len) {
		charModels_.clear();
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
			charModels_.push_back(std::move(model));
		}
	}

	float screenWidth = static_cast<float>(WindowsAPI::GetInstance()->GetWindowWidth());
	float screenHeight = static_cast<float>(WindowsAPI::GetInstance()->GetWindowHeight());
	Matrix4x4 projection = Math::MakeOrthographicMatrix(0.0f, 0.0f, screenWidth, screenHeight, -1.0f, 100.0f);

	float charW = size_.x * scale_.x;
	float charH = size_.y * scale_.y;
	float totalWidth = charW * len + spacing_ * (len - 1);

	float startX = 0.0f;
	if (alignment_ == Alignment::Left) {
		startX = 0.0f;
	} else if (alignment_ == Alignment::Center) {
		startX = -totalWidth / 2.0f + charW / 2.0f;
	} else if (alignment_ == Alignment::Right) {
		startX = -totalWidth + charW / 2.0f;
	}

	float anchorOffsetX = totalWidth * (0.5f - anchorPoint_.x);
	float anchorOffsetY = charH * (0.5f - anchorPoint_.y);

	float baseX = position_.x + gameObject_->GetTransform().translate.x + anchorOffsetX;
	float baseY = position_.y + gameObject_->GetTransform().translate.y + anchorOffsetY;

	for (size_t i = 0; i < len; ++i) {
		auto& model = charModels_[i];
		if (!model) continue;

		char c = text_[i];
		int charIndex = 0;
		if (c >= '0' && c <= '9') {
			charIndex = c - '0';
		} else if (c >= 'A' && c <= 'Z') {
			charIndex = (c - 'A') % 10;
		} else if (c >= 'a' && c <= 'z') {
			charIndex = (c - 'a') % 10;
		} else {
			charIndex = 0;
		}

		if (auto mat = model->GetMaterial()) {
			mat->SetTextureIndex(texIndex_);
			mat->SetColor(color_);
			mat->SetUvScale({ -0.1f, 1.0f, 1.0f });
			mat->SetUvTranslate({ 0.1f * charIndex + 0.1f, 0.0f, 0.0f });
		}

		Vector3 finalScale = { charW / 2.0f, -charH / 2.0f, 1.0f };
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
