#include "PCH.h"
#include "FontTextComponent.h"
#include "GameObject.h"
#include "BaseScene.h"
#include "FontManager.h"
#include "WindowsAPI.h"
#include "MathFunction.h"
#include "Material.h"
#include "GraphicsDevice.h"
#include "RenderCollector.h"

FontTextComponent::FontTextComponent() = default;

void FontTextComponent::Initialize() {
	if (isInitialized_) return;

	GameObject* owner = GetGameObject();
	if (!owner) return;
	SceneContext* context = owner->GetContext();
	if (!context || !context->graphicsDevice) return;
	isInitialized_ = true;

	ID3D12Device* device = context->graphicsDevice->GetDevice();

	// 1. 頂点バッファは最大文字数ぶん確保しておき、文字列が変わったら中身だけ書き換える
	vertexBuffer_.Initialize(device, kMaxQuads * 4);

	// インデックスは「四角形1枚 = 三角形2枚」の並びが毎回同じなので、最初に全部作っておく
	std::vector<uint32_t> indices;
	indices.reserve(kMaxQuads * 6);
	for (uint32_t i = 0; i < kMaxQuads; ++i) {
		const uint32_t base = i * 4;
		// 0:左上 1:右上 2:左下 3:右下 (SpriteComponent と同じ並び)
		indices.insert(indices.end(), { base + 0, base + 1, base + 2, base + 1, base + 3, base + 2 });
	}
	indexBuffer_.Initialize(device, indices.size());
	indexBuffer_.Update(indices);

	mesh_.vbView = vertexBuffer_.GetView();
	mesh_.ibView = indexBuffer_.GetView();
	mesh_.inputLayout = MyEngine::Rendering::InputLayoutType::Standard3D;

	// 2. マテリアル & トランスフォームバッファの初期化
	material_ = std::make_shared<MyEngine::Rendering::Material>();
	material_->Initialize(context->graphicsDevice, context->heapManager);
	material_->SetDepthEnable(false); // 2Dなのでデプス無効
	material_->SetDepthWrite(false);
	material_->SetBlendMode(MyEngine::Rendering::BlendModeType::Alpha); // 半透明
	material_->SetShadingModel(MyEngine::Rendering::ShadingModel::Text); // テキスト専用シェーダー
	material_->SetLayer(layer_);
	material_->SetDoubleSided(true);
	material_->SetColor(color_);
	material_->SetEnableLighting(FALSE);

	transformBuffer_.Initialize(device);
	effectBuffer_.Initialize(device);

	// 3. フォントを用意
	ApplyFont();
}

void FontTextComponent::Update() {
	if (!material_ || !gameObject_) return;

	// 頂点は「焼いたサイズ」の px で並んでいるので、表示サイズとの比でスケールする
	const float scale = (bakeSize_ > 0.0f) ? fontSize_ / bakeSize_ : 1.0f;

	// 影のずれは頂点に焼き込んでいるので、表示サイズや設定が変わったら並べ直す
	const Vector2 shadowOffsetInBake = { shadowOffset_.x / scale, shadowOffset_.y / scale };
	if (shadowEnabled_ && (shadowOffsetInBake.x != builtShadowOffset_.x || shadowOffsetInBake.y != builtShadowOffset_.y)) {
		isDirty_ = true;
	}

	// 文字列などが変わっていたら、四角形を並べ直す
	if (isDirty_) {
		RebuildMesh();
	}

	// 画面サイズ取得（1280 x 720）
	float screenWidth = static_cast<float>(WindowsAPI::GetInstance()->GetWindowWidth());
	float screenHeight = static_cast<float>(WindowsAPI::GetInstance()->GetWindowHeight());
	if (screenWidth <= 0.0f) screenWidth = 1280.0f;
	if (screenHeight <= 0.0f) screenHeight = 720.0f;

	// 1. アンカーポイントのオフセット（箱の大きさ × アンカー）
	Matrix4x4 matAnchor = Math::MakeTranslateMatrix({ -anchorPoint_.x * boxSize_.x, -anchorPoint_.y * boxSize_.y, 0.0f });

	// 2. スケール
	Matrix4x4 matScale = Math::MakeScaleMatrix({ scale, scale, 1.0f });

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

	// 縁取り・影の設定 (px → SDF の値の単位)
	if (font_ && font_->IsSDF()) {
		// SDF の値は 0〜1 で、輪郭(0.5)から padding px 離れると 0 になる
		// → 焼いたサイズで 1px あたり 0.5 / padding 変わる
		const float padding = static_cast<float>(font_->GetSdfPadding());
		const float unitPerBakePx = 0.5f / padding;
		// 縁取り・ぼかしは padding の内側でしか作れないので、少し手前で止める
		const float maxUnit = 0.5f * (padding - 1.0f) / padding;

		TextEffectData effect{};
		effect.outlineColor = outlineColor_;
		effect.shadowColor = shadowColor_;
		effect.outlineWidth = (std::min)(outlineWidth_ / scale * unitPerBakePx, maxUnit);
		effect.shadowSoftness = (std::min)(shadowSoftness_ / scale * unitPerBakePx, maxUnit - effect.outlineWidth);
		effectBuffer_.Update(effect);
	}
}

void FontTextComponent::CollectRenderData(MyEngine::Rendering::RenderCollector* collector) const {
	if (!isVisible_ || !material_ || !collector || !font_) return;
	if (mesh_.indexCount == 0) return; // 表示する文字が無い

	MyEngine::Rendering::RenderItem item{};
	item.mesh = &mesh_;
	item.material = material_.get();
	item.transformAddr = transformBuffer_.GetGPUVirtualAddress();
	// SDF の時は縁取り・影の設定を b4 に渡す
	item.customAddr = font_->IsSDF() ? effectBuffer_.GetGPUVirtualAddress() : 0;
	item.debugName = gameObject_ ? gameObject_->GetName().c_str() : "FontText";

	collector->Submit(item);
}

void FontTextComponent::SetText(const std::string& text) {
	if (text_ == text) return;
	text_ = text;
	isDirty_ = true;
}

void FontTextComponent::SetUseSDF(bool useSDF) {
	if (useSDF_ == useSDF) return;
	useSDF_ = useSDF;
	ApplyFont();
}

void FontTextComponent::SetFont(const std::string& fontPath, float bakeSize) {
	fontPath_ = fontPath;
	bakeSize_ = bakeSize;
	ApplyFont();
}

Vector2 FontTextComponent::GetTextSize() const {
	const float scale = (bakeSize_ > 0.0f) ? fontSize_ / bakeSize_ : 1.0f;
	return { boxSize_.x * scale, boxSize_.y * scale };
}

void FontTextComponent::ApplyFont() {
	if (!gameObject_ || !gameObject_->GetContext()) return;
	FontManager* fontManager = gameObject_->GetContext()->fontManager;
	if (!fontManager) return;

	font_ = fontManager->GetFont(fontPath_, bakeSize_, useSDF_);
	if (font_ && material_) {
		material_->SetTextureIndex(font_->GetTextureIndex());
		// 焼き方に合わせてシェーダーを切り替える
		material_->SetShadingModel(font_->IsSDF()
			? MyEngine::Rendering::ShadingModel::TextSDF
			: MyEngine::Rendering::ShadingModel::Text);
	}
	isDirty_ = true;
}

void FontTextComponent::RebuildMesh() {
	isDirty_ = false;
	mesh_.vertexCount = 0;
	mesh_.indexCount = 0;
	boxSize_ = { 0.0f, 0.0f };
	if (!font_) return;

	const std::u32string chars = Font::DecodeUTF8(text_);

	// 1. 改行で行に分けて、各行の幅を測る（揃え位置を決めるため）
	std::vector<std::u32string> lines(1);
	for (char32_t c : chars) {
		if (c == U'\n') {
			lines.emplace_back();
		} else if (c != U'\r') {
			lines.back().push_back(c);
		}
	}

	std::vector<float> lineWidths;
	lineWidths.reserve(lines.size());
	float maxWidth = 0.0f;
	for (const auto& line : lines) {
		float width = 0.0f;
		for (size_t i = 0; i < line.size(); ++i) {
			if (const Glyph* glyph = font_->GetGlyph(line[i])) {
				width += glyph->advance;
			}
			if (i + 1 < line.size()) width += letterSpacing_;
		}
		lineWidths.push_back(width);
		maxWidth = (std::max)(maxWidth, width);
	}

	const float lineAdvance = font_->GetLineHeight() * lineSpacing_;
	boxSize_ = { maxWidth, lineAdvance * static_cast<float>(lines.size()) };

	// 2. 各文字の四角形を並べる
	//    ペン位置は「ベースライン上の、次の文字を書き始める点」
	//    四角形の左上 = ペン位置 + bearing、大きさ = glyph.size
	struct Quad {
		float x0, y0, x1, y1;
		Vector2 uvMin, uvMax;
	};
	std::vector<Quad> quads;
	quads.reserve(chars.size());
	bool overflowed = false;

	for (size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
		const auto& line = lines[lineIndex];

		// 揃え方に応じて行の書き始めをずらす
		float penX = 0.0f;
		if (alignment_ == Alignment::Center) {
			penX = (maxWidth - lineWidths[lineIndex]) * 0.5f;
		} else if (alignment_ == Alignment::Right) {
			penX = maxWidth - lineWidths[lineIndex];
		}
		// ベースラインは「行の上端 + ascent」
		const float baselineY = lineAdvance * static_cast<float>(lineIndex) + font_->GetAscent();

		for (char32_t c : line) {
			const Glyph* glyph = font_->GetGlyph(c);
			if (!glyph) continue;

			// スペースなど見た目の無い文字は、ペンを進めるだけ
			if (glyph->size.x > 0.0f && glyph->size.y > 0.0f) {
				if (quads.size() >= kMaxChars) {
					overflowed = true;
					break;
				}

				Quad quad{};
				quad.x0 = penX + glyph->bearing.x;
				quad.y0 = baselineY + glyph->bearing.y;
				quad.x1 = quad.x0 + glyph->size.x;
				quad.y1 = quad.y0 + glyph->size.y;
				quad.uvMin = glyph->uvMin;
				quad.uvMax = glyph->uvMax;
				quads.push_back(quad);
			}

			penX += glyph->advance + letterSpacing_;
		}
		if (overflowed) break;
	}

	if (overflowed) {
		Logger::GetInstance()->Log("FontTextComponent: 文字数が上限(" + std::to_string(kMaxChars) + ")を超えたので切り捨てました");
	}

	// 3. 四角形を頂点にする
	//    法線の x は「影かどうか」の印として TextSDF シェーダーに渡す
	std::vector<VertexData> vertices;
	vertices.reserve(quads.size() * 8);
	auto pushQuad = [&vertices](const Quad& q, float offsetX, float offsetY, float shadowFlag) {
		const Vector3 normal = { shadowFlag, 0.0f, -1.0f };
		const float x0 = q.x0 + offsetX, y0 = q.y0 + offsetY;
		const float x1 = q.x1 + offsetX, y1 = q.y1 + offsetY;
		vertices.push_back({ { x0, y0, 0.0f, 1.0f }, { q.uvMin.x, q.uvMin.y }, normal }); // 左上
		vertices.push_back({ { x1, y0, 0.0f, 1.0f }, { q.uvMax.x, q.uvMin.y }, normal }); // 右上
		vertices.push_back({ { x0, y1, 0.0f, 1.0f }, { q.uvMin.x, q.uvMax.y }, normal }); // 左下
		vertices.push_back({ { x1, y1, 0.0f, 1.0f }, { q.uvMax.x, q.uvMax.y }, normal }); // 右下
	};

	// 影は本体より先に並べる（同じ描画の中では先の三角形から描かれるので、影が下になる）
	const float scale = (bakeSize_ > 0.0f) ? fontSize_ / bakeSize_ : 1.0f;
	builtShadowOffset_ = { shadowOffset_.x / scale, shadowOffset_.y / scale };
	if (shadowEnabled_ && font_->IsSDF()) {
		for (const Quad& q : quads) pushQuad(q, builtShadowOffset_.x, builtShadowOffset_.y, 1.0f);
	}
	for (const Quad& q : quads) pushQuad(q, 0.0f, 0.0f, 0.0f);

	// 4. 頂点バッファへ書き込み、描画する数を更新
	const uint32_t quadCount = static_cast<uint32_t>(vertices.size() / 4);
	vertexBuffer_.Update(vertices);
	mesh_.vertexCount = quadCount * 4;
	mesh_.indexCount = quadCount * 6;
}

void FontTextComponent::ImGui() {
#ifdef USEIMGUI
	// テキスト（複数行OK）
	char textBuf[1024];
	strncpy_s(textBuf, text_.c_str(), _TRUNCATE);
	if (ImGui::InputTextMultiline("Text", textBuf, sizeof(textBuf), ImVec2(0.0f, ImGui::GetTextLineHeight() * 4.0f))) {
		SetText(textBuf);
	}

	// フォントファイルと焼くサイズ（変更すると焼き直すので、確定したときだけ反映）
	char fontBuf[256];
	strncpy_s(fontBuf, fontPath_.c_str(), _TRUNCATE);
	if (ImGui::InputText("Font Path", fontBuf, sizeof(fontBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
		SetFont(fontBuf, bakeSize_);
	}
	ImGui::DragFloat("Bake Size", &bakeSize_, 1.0f, 8.0f, 256.0f);
	if (ImGui::IsItemDeactivatedAfterEdit()) {
		ApplyFont();
	}
	bool useSDF = useSDF_;
	if (ImGui::Checkbox("SDF", &useSDF)) {
		SetUseSDF(useSDF);
	}
	if (!font_) {
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Font not loaded");
	}

	ImGui::DragFloat2("Position", &position_.x, 1.0f);
	ImGui::DragFloat("Font Size", &fontSize_, 0.5f, 1.0f, 512.0f);
	ImGui::DragFloat("Rotation", &rotation_, 0.01f);

	float colorArr[4] = { color_.x, color_.y, color_.z, color_.w };
	if (ImGui::ColorEdit4("Color", colorArr)) {
		color_ = { colorArr[0], colorArr[1], colorArr[2], colorArr[3] };
	}

	ImGui::DragFloat2("AnchorPoint", &anchorPoint_.x, 0.05f, 0.0f, 1.0f);

	const char* items[] = { "Left", "Center", "Right" };
	int currentAlign = static_cast<int>(alignment_);
	if (ImGui::Combo("Alignment", &currentAlign, items, IM_ARRAYSIZE(items))) {
		SetAlignment(static_cast<Alignment>(currentAlign));
	}

	if (ImGui::DragFloat("Letter Spacing", &letterSpacing_, 0.1f, -50.0f, 100.0f)) isDirty_ = true;
	if (ImGui::DragFloat("Line Spacing", &lineSpacing_, 0.01f, 0.1f, 5.0f)) isDirty_ = true;

	int layerVal = layer_;
	if (ImGui::DragInt("Layer", &layerVal, 1.0f, 0, 255)) {
		layer_ = static_cast<uint8_t>(layerVal);
	}

	// 縁取り・影（SDF の時だけ）
	if (useSDF_ && ImGui::TreeNode("Outline / Shadow")) {
		ImGui::ColorEdit4("Outline Color", &outlineColor_.x);
		ImGui::DragFloat("Outline Width", &outlineWidth_, 0.1f, 0.0f, 20.0f);

		if (ImGui::Checkbox("Shadow", &shadowEnabled_)) isDirty_ = true;
		if (shadowEnabled_) {
			ImGui::ColorEdit4("Shadow Color", &shadowColor_.x);
			ImGui::DragFloat2("Shadow Offset", &shadowOffset_.x, 0.5f);
			ImGui::DragFloat("Shadow Softness", &shadowSoftness_, 0.1f, 0.0f, 20.0f);
		}
		ImGui::TreePop();
	}
#endif
}

void FontTextComponent::Serialize(json& j) const {
	j["type"] = "FontTextComponent";
	j["fontPath"] = fontPath_;
	j["bakeSize"] = bakeSize_;
	j["text"] = text_;
	j["position"] = { position_.x, position_.y };
	j["fontSize"] = fontSize_;
	j["rotation"] = rotation_;
	j["color"] = { color_.x, color_.y, color_.z, color_.w };
	j["anchorPoint"] = { anchorPoint_.x, anchorPoint_.y };
	j["alignment"] = static_cast<int>(alignment_);
	j["letterSpacing"] = letterSpacing_;
	j["lineSpacing"] = lineSpacing_;
	j["layer"] = layer_;
	j["isVisible"] = isVisible_;
	j["useSDF"] = useSDF_;
	j["outlineColor"] = { outlineColor_.x, outlineColor_.y, outlineColor_.z, outlineColor_.w };
	j["outlineWidth"] = outlineWidth_;
	j["shadowEnabled"] = shadowEnabled_;
	j["shadowColor"] = { shadowColor_.x, shadowColor_.y, shadowColor_.z, shadowColor_.w };
	j["shadowOffset"] = { shadowOffset_.x, shadowOffset_.y };
	j["shadowSoftness"] = shadowSoftness_;
}

void FontTextComponent::Deserialize(const json& j) {
	if (j.contains("fontPath")) fontPath_ = j["fontPath"];
	if (j.contains("bakeSize")) bakeSize_ = j["bakeSize"];
	if (j.contains("text")) text_ = j["text"];
	if (j.contains("position")) position_ = { j["position"][0], j["position"][1] };
	if (j.contains("fontSize")) fontSize_ = j["fontSize"];
	if (j.contains("rotation")) rotation_ = j["rotation"];
	if (j.contains("color")) color_ = { j["color"][0], j["color"][1], j["color"][2], j["color"][3] };
	if (j.contains("anchorPoint")) anchorPoint_ = { j["anchorPoint"][0], j["anchorPoint"][1] };
	if (j.contains("alignment")) alignment_ = static_cast<Alignment>(j["alignment"].get<int>());
	if (j.contains("letterSpacing")) letterSpacing_ = j["letterSpacing"];
	if (j.contains("lineSpacing")) lineSpacing_ = j["lineSpacing"];
	if (j.contains("layer")) layer_ = j["layer"];
	if (j.contains("isVisible")) isVisible_ = j["isVisible"];
	if (j.contains("useSDF")) useSDF_ = j["useSDF"];
	if (j.contains("outlineColor")) outlineColor_ = { j["outlineColor"][0], j["outlineColor"][1], j["outlineColor"][2], j["outlineColor"][3] };
	if (j.contains("outlineWidth")) outlineWidth_ = j["outlineWidth"];
	if (j.contains("shadowEnabled")) shadowEnabled_ = j["shadowEnabled"];
	if (j.contains("shadowColor")) shadowColor_ = { j["shadowColor"][0], j["shadowColor"][1], j["shadowColor"][2], j["shadowColor"][3] };
	if (j.contains("shadowOffset")) shadowOffset_ = { j["shadowOffset"][0], j["shadowOffset"][1] };
	if (j.contains("shadowSoftness")) shadowSoftness_ = j["shadowSoftness"];

	// AddComponent の時点で Initialize 済みなら、読み込んだフォント設定で取り直す
	Initialize();
	ApplyFont();
}
