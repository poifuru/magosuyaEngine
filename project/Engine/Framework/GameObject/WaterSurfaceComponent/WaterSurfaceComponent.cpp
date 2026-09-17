#include "PCH.h"
#include "WaterSurfaceComponent.h"
#include "BaseScene.h"
#include "GameObject.h"
#include "GraphicsDevice.h"
#include "ComponentType.h"
#include "TextureManager.h"

void WaterSurfaceComponent::Initialize() {
	if(isInitialized_) return;

	Component::Initialize();

	GenerateMesh();

	// 波のパラメータ初期化
	// 波1: 大きくゆっくり進む波
	waves_[0].amplitude = 0.3f;
	waves_[0].frequency = 0.4f;
	waves_[0].steepness = 0.5f;
	waves_[0].direction = Vector2(1.0f, 1.0f);
	// 方向ベクトルの正規化
	{
		float len = std::sqrt(waves_[0].direction.x * waves_[0].direction.x + waves_[0].direction.y * waves_[0].direction.y);
		if(len > 0.001f) {
			waves_[0].direction.x /= len;
			waves_[0].direction.y /= len;
		}
	}
	// 波2: 細かくて少し速いクロスする波
	waves_[1].amplitude = 0.1f;
	waves_[1].frequency = 1.2f;
	waves_[1].steepness = 0.3f;
	waves_[1].direction = Vector2(-1.0f, 1.0f);
	{
		float len = std::sqrt(waves_[1].direction.x * waves_[1].direction.x + waves_[1].direction.y * waves_[1].direction.y);
		if(len > 0.001f) {
			waves_[1].direction.x /= len;
			waves_[1].direction.y /= len;
		}
	}
	// 残りの波は初期値0（フラット）にしておく
	for(int i = 2; i < 4; ++i) {
		waves_[i].amplitude = 0.0f;
		waves_[i].frequency = 1.0f;
		waves_[i].steepness = 0.0f;
		waves_[i].direction = Vector2(1.0f, 0.0f);
	}
}

void WaterSurfaceComponent::Update() {
	Component::Update();

	// 時間を進める
	time_ += Time::GetDeltaTime();

	// マテリアルデータを更新する
	if(material_) {
		material_->SetTime(time_);
		material_->SetRoughness(0.1f);
		material_->Update();
	}

	// GPU送信用パラメータの構築
	WaterSurfaceForGPU params{};
	params.time = time_;
	params.numActiveWaves = numActiveWaves_;
	for(int i = 0; i < 4; ++i) {
		params.waves[i] = waves_[i];
	}
	params.nearFadeDistance = nearFadeDistance_;
	params.farFadeDistance = farFadeDistance_;

	// シーン内のオブジェクトから BoatWakeComponent を探す
	auto* wakeComp = GetGameObject()->FindComponentInScene<BoatWakeComponent>();

	if(wakeComp) {
		// 見つかった BoatWakeComponent から CS 波紋テクスチャのインデックスを取得！
		params.rippleTextureIndex = wakeComp->GetRippleTextureIndex();
	}

	params.waterMin = GetWaterMin();
	params.waterSize = GetWaterSize();

	// 定数バッファを更新
	waterSurfaceBuffer_.Update(params);
}

void WaterSurfaceComponent::ImGui() {
	Component::ImGui();

	// マテリアルの編集
	if (material_ && ImGui::CollapsingHeader("Material Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
		auto matData = material_->GetMaterialData();
		auto uvTrans = material_->GetUvTransform();
		// ベースカラー
		if (ImGui::ColorEdit4("Water Color", &matData.color.x)) {
			material_->SetColor(matData.color);
		}
		// Roughness（ツルツル具合）
		if (ImGui::SliderFloat("Roughness", &matData.roughness, 0.0f, 1.0f)) {
			material_->SetRoughness(matData.roughness);
		}
		// Metallic（金属感/反射強度）
		if (ImGui::SliderFloat("Metallic", &matData.metallic, 0.0f, 1.0f)) {
			material_->SetMetallic(matData.metallic);
		}
		// UV タイリング（テクスチャの繰り返し数）
		if (ImGui::DragFloat3("UV Scale", &uvTrans.scale.x, 0.01f)) {
			material_->SetUvScale(uvTrans.scale);
		}
		if (ImGui::DragFloat3("UV Offset", &uvTrans.translate.x, 0.01f)) {
			material_->SetUvTranslate(uvTrans.translate);
		}
	}
	ImGui::Separator();

	// テクスチャパスの表示と変更
	char texBuf[256];
	strcpy_s(texBuf, texPath_.c_str());
	if (ImGui::InputText("Texture Path", texBuf, sizeof(texBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
		SetTexture(texBuf);
	}
	ImGui::Spacing();

	ImGui::Separator();
	ImGui::Text("Mesh Info");

	// メッシュの情報を編集
	ImGui::DragFloat("Width", &width_, 1.0f, 0.0f, 1000.0f);
	if (ImGui::IsItemDeactivatedAfterEdit()) GenerateMesh();
	ImGui::DragFloat("Depth", &depth_, 1.0f, 0.0f, 1000.0f);
	if (ImGui::IsItemDeactivatedAfterEdit()) GenerateMesh();
	ImGui::DragInt("SubdivisionX", &subdivisionX_, 1, 1, 1000);
	if (ImGui::IsItemDeactivatedAfterEdit()) GenerateMesh();
	ImGui::DragInt("SubdivisionZ", &subdivisionZ_, 1, 1, 1000);
	if (ImGui::IsItemDeactivatedAfterEdit()) GenerateMesh();

	ImGui::Separator();
	ImGui::Text("Gerstner Wave System");
	// 有効にする波の数 (1〜4)
	ImGui::SliderInt("Active Waves Count", &numActiveWaves_, 1, 4);
	// 各波のパラメータを編集
	for(int i = 0; i < numActiveWaves_; ++i) {
		ImGui::PushID(i); // IDの衝突を防ぐため、波のインデックスをPush

		char label[64];
		sprintf_s(label, "Wave %d", i + 1);
		if(ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::DragFloat("Amplitude (A)", &waves_[i].amplitude, 0.01f, 0.0f, 5.0f);
			ImGui::DragFloat("Frequency (w)", &waves_[i].frequency, 0.01f, 0.0f, 5.0f);
			ImGui::DragFloat("Steepness (Q)", &waves_[i].steepness, 0.01f, 0.0f, 1.0f);

			if(ImGui::DragFloat2("Direction (D)", &waves_[i].direction.x, 0.01f, -1.0f, 1.0f)) {
				// 方向ベクトルを変更したら必ず正規化する
				float len = std::sqrt(waves_[i].direction.x * waves_[i].direction.x + waves_[i].direction.y * waves_[i].direction.y);
				if(len > 0.001f) {
					waves_[i].direction.x /= len;
					waves_[i].direction.y /= len;
				}
			}
		}

		ImGui::PopID();
	}

	ImGui::Separator();
	ImGui::Text("Water Transparency Fade");
	ImGui::DragFloat("Near Fade Distance", &nearFadeDistance_, 1.0f, 0.0f, 1000.0f);
	ImGui::DragFloat("Far Fade Distance", &farFadeDistance_, 1.0f, 0.0f, 1000.0f);
}

void WaterSurfaceComponent::Serialize(json& j) const {
	Component::Serialize(j);
	j["type"] = "WaterSurfaceComponent";
	j["numActiveWaves"] = numActiveWaves_;

	j["TexturePath"] = texPath_;

	j["Width"] = width_;
	j["Depth"] = depth_;
	j["SubdivisionX"] = subdivisionX_;
	j["SubdivisionZ"] = subdivisionZ_;

	json wavesJ = json::array();
	for(int i = 0; i < 4; ++i) {
		json w;
		w["amplitude"] = waves_[i].amplitude;
		w["frequency"] = waves_[i].frequency;
		w["steepness"] = waves_[i].steepness;
		w["direction"] = { waves_[i].direction.x, waves_[i].direction.y };
		wavesJ.push_back(w);
	}
	j["waves"] = wavesJ;

	j["nearFadeDistance"] = nearFadeDistance_;
	j["farFadeDistance"] = farFadeDistance_;
}

void WaterSurfaceComponent::Deserialize(const json& j) {
	Component::Deserialize(j);

	if(j.contains("TexturePath")) texPath_ = j["TexturePath"];

	if(j.contains("Width")) width_ = j["Width"];
	if(j.contains("Depth")) depth_ = j["Depth"];
	if(j.contains("SubdivisionX")) subdivisionX_ = j["SubdivisionX"];
	if(j.contains("SubdivisionZ")) subdivisionZ_ = j["SubdivisionZ"];

	// 定数バッファを確実に初期化する 
	auto* device = GetGameObject()->GetContext()->graphicsDevice->GetDevice();
	waterSurfaceBuffer_.Initialize(device);

	if(j.contains("numActiveWaves")) {
		numActiveWaves_ = j["numActiveWaves"];
	}
	if(j.contains("waves") && j["waves"].is_array()) {
		const auto& wavesJ = j["waves"];
		int count = std::min(4, (int)wavesJ.size());
		for(int i = 0; i < count; ++i) {
			const auto& wJ = wavesJ[i];
			if(wJ.contains("amplitude")) waves_[i].amplitude = wJ["amplitude"];
			if(wJ.contains("frequency")) waves_[i].frequency = wJ["frequency"];
			if(wJ.contains("steepness")) waves_[i].steepness = wJ["steepness"];
			if(wJ.contains("direction")) {
				waves_[i].direction.x = wJ["direction"][0];
				waves_[i].direction.y = wJ["direction"][1];
			}
		}
	}
	if(j.contains("nearFadeDistance")) {
		nearFadeDistance_ = j["nearFadeDistance"];
	}
	if(j.contains("farFadeDistance")) {
		farFadeDistance_ = j["farFadeDistance"];
	}

	GenerateMesh();

	isInitialized_ = true;
}

void WaterSurfaceComponent::SetTexture(const std::string& textureName) {
	GameObject* owner = GetGameObject();

	if (!owner) return;

	SceneContext* context = owner->GetContext();

	if (!context || !context->textureManager) return;

	texPath_ = textureName;
	// TextureManager からテクスチャをロードしてインデックスを取得
	texIndex_ = context->textureManager->LoadTexture(texPath_);
	// マテリアルにテクスチャインデックスをセット
	if (material_) {
		material_->SetTextureIndex(texIndex_);
	}
}

Vector2 WaterSurfaceComponent::GetWaterMin() const {
	if(!gameObject_) return Vector2(0.0f, 0.0f);	// フォールバック

	Vector3 pos = gameObject_->GetTransform().translate;
	Vector3 scale = gameObject_->GetTransform().scale;

	return Vector2(pos.x - (width_ * 0.5f) * scale.x, pos.z - (depth_ * 0.5f) * scale.z);
}

Vector2 WaterSurfaceComponent::GetWaterSize() const {
	if(!gameObject_) return Vector2(1.0f, 1.0f);

	Vector3 scale = gameObject_->GetTransform().scale;

	return Vector2(width_ * scale.x, depth_ * scale.z);
}

void WaterSurfaceComponent::GenerateMesh() {
	// 頂点とインデックスのコンテナを用意
	std::vector<VertexData> vertices;
	std::vector<uint32_t> indices;

	int verCountX = subdivisionX_ + 1;
	int verCountZ = subdivisionZ_ + 1;

	float dx = width_ / static_cast<float>(subdivisionX_);
	float dz = depth_ / static_cast<float>(subdivisionZ_);
	float halfW = width_ * 0.5f;
	float halfD = depth_ * 0.5f;

	// 頂点データの生成
	for(int z = 0; z < verCountZ; ++z) {
		float posZ = -halfD + z * dz;
		float v = static_cast<float>(z) / static_cast<float>(subdivisionZ_);

		for(int x = 0; x < verCountX; ++x) {
			float posX = -halfW + x * dx;
			float u = static_cast<float>(x) / static_cast<float>(subdivisionX_);

			VertexData vert{};
			vert.position = Vector4(posX, 0.0f, posZ, 1.0f);
			vert.texcoord = Vector2(u, v);
			vert.normal = Vector3(0.0f, 1.0f, 0.0f); // 上向き初期値
			vertices.push_back(vert);
		}
	}

	// インデックスの生成
	for(int z = 0; z < subdivisionZ_; ++z) {
		for(int x = 0; x < subdivisionX_; ++x) {
			uint32_t i0 = z * verCountX + x;             // 左下
			uint32_t i1 = z * verCountX + (x + 1);       // 右下
			uint32_t i2 = (z + 1) * verCountX + x;       // 左上
			uint32_t i3 = (z + 1) * verCountX + (x + 1); // 右上

			// 三角形1 (左下 -> 左上 -> 右下)
			indices.push_back(i0);
			indices.push_back(i2);
			indices.push_back(i1);

			// 三角形2 (右下 -> 左上 -> 右上)
			indices.push_back(i1);
			indices.push_back(i2);
			indices.push_back(i3);
		}
	}

	// 機能の取得
	auto* device = GetGameObject()->GetContext()->graphicsDevice;
	auto* heapManager = GetGameObject()->GetContext()->heapManager;

	// Meshの初期化
	mesh_ = std::make_unique<MyEngine::Rendering::StaticMesh>();
	mesh_->Initialize(device->GetDevice(), vertices, indices);

	// Materialの初期化
	material_ = std::make_unique<MyEngine::Rendering::Material>();
	material_->Initialize(device, heapManager);
	material_->SetShadingModel(MyEngine::Rendering::ShadingModel::WaterSurface);
	material_->SetBlendMode(MyEngine::Rendering::BlendModeType::Alpha);
	SetTexture(texPath_);
}