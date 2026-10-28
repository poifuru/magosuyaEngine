#include "PCH.h"
#include "GameObject.h"
#include "Component.h"
#include "ComponentType.h"
#include "GraphicsDevice.h"
#include "CameraOrganizer.h"
#include "EditorManager.h"
#include "ComponentRegistry.h"

GameObject::GameObject(SceneContext* context, const std::string& name)
	: context_(context), name_(name) {
	// トランスフォームの初期化
	transform_.scale = { 1.0f, 1.0f, 1.0f };
	transform_.rotate = { 0.0f, 0.0f, 0.0f };
	transform_.translate = { 0.0f, 0.0f, 0.0f };

	auto* device = context_->graphicsDevice->GetDevice();
	transformBuffer_.Initialize(device);
}

GameObject::~GameObject() {
#ifdef USEIMGUI
	// 破棄される自分がエディタで選択されていたら、選択をクリアする
	if (EditorManager::GetInstance()->GetSelectedObject() == this) {
		EditorManager::GetInstance()->ClearSelectedObject();
	}
#endif
}

void GameObject::Initialize() {
	for (auto& component : components_) {
		component->Initialize();
	}
}

void GameObject::DrawDebug() {
	for (auto& component : components_) {
		component->DrawDebug();
	}
}

void GameObject::Update() {
	for (auto& component : components_) {
		component->Update();
	}

	// アフィン行列作成
	Matrix4x4 world = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	
	// カメラデータを取得
	const CameraData& cameraData = CameraOrganizer::GetInstance()->GetCameraData();

	TransformMatrixData transformData{};
	transformData.World = world;
	transformData.WVP = Math::Multiply(world, cameraData.vp);
	transformData.WorldInverseTranspose = Math::MakeIdentity4x4();

	// バッファ更新
	transformBuffer_.Update(transformData);
}

void GameObject::UpdateTransformBuffer() {
	// アフィン行列作成
	Matrix4x4 world = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

	// カメラデータを取得
	const CameraData& cameraData = CameraOrganizer::GetInstance()->GetCameraData();
	TransformMatrixData transformData{};
	transformData.World = world;
	transformData.WVP = Math::Multiply(world, cameraData.vp);
	transformData.WorldInverseTranspose = Math::MakeIdentity4x4();

	// バッファ更新
	transformBuffer_.Update(transformData);

	// --- 描画用コンポーネントのバッファも最新のカメラ行列で更新 ---
	if (auto* meshRenderer = GetComponent<MyEngine::Rendering::MeshRendererComponent>()) {
		meshRenderer->Update();
	}
	if (auto* skybox = GetComponent<SkyboxComponent>()) {
		skybox->Update();
	}
	if (auto* sprite = GetComponent<SpriteComponent>()) {
		sprite->Update();
	}
	if (auto* fontText = GetComponent<FontTextComponent>()) {
		fontText->Update();
	}
	if (auto* water = GetComponent<WaterSurfaceComponent>()) {
		water->Update();
	}
	if (auto* buoys = GetComponent<CourseBuoyComponent>()) {
		buoys->Update();
	}
	if (auto* dolphin = GetComponent<DolphinEventComponent>()) {
		dolphin->UpdateModel();
	}
}

void GameObject::ImGui() {
	// オブジェクト名の編集
	char nameBuf[128];
	strcpy_s(nameBuf, name_.c_str());
	if(ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
		name_ = nameBuf;
		std::string newName = nameBuf;
		// 空文字やスペース/タブだけの名前は反映させない（元の名前を維持する）
		if(!newName.empty() && newName.find_first_not_of(" \t\r\n") != std::string::npos) {
			name_ = newName;
		}
	}

	ImGui::Separator();

	// Transformの編集（デフォルトで常に表示）
	if(ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragFloat3("Position", &transform_.translate.x, 0.1f);
		ImGui::DragFloat3("Rotation", &transform_.rotate.x, 0.1f);
		ImGui::DragFloat3("Scale", &transform_.scale.x, 0.1f);
	}

	// 各コンポーネントのImGui描画（折りたたみヘッダーで表示）
	for(auto it = components_.begin(); it != components_.end(); ) {
		auto& component = *it;
		bool isDeleted = false;

		ImGui::PushID(component.get());
		bool open = false;
		// 2列のテーブルを作成（境界線なし、幅は自動引き伸ばし）
		if(ImGui::BeginTable("ComponentHeaderTable", 2, ImGuiTableFlags_NoSavedSettings)) {
			// 1列目（ヘッダー用）は引き伸ばし、2列目（ボタン用）は25ピクセル固定
			ImGui::TableSetupColumn("Header", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Button", ImGuiTableColumnFlags_WidthFixed, 25.0f);
			ImGui::TableNextRow();
			// 1列目に移動してヘッダーを描画
			ImGui::TableNextColumn();
			open = ImGui::CollapsingHeader(component->GetName(), ImGuiTreeNodeFlags_DefaultOpen);
			// 2列目に移動してXボタンを描画
			ImGui::TableNextColumn();
			if(ImGui::Button("X", ImVec2(20, 20))) {
				isDeleted = true;
			}
			ImGui::EndTable();
		}
		// ヘッダーの中身はテーブルの外側でウィンドウ幅いっぱいに描画する
		if(open) {
			component->ImGui();
		}
		ImGui::PopID();


		// 安全に要素を削除してループを回す
		if(isDeleted) {
			it = components_.erase(it);
		}
		else {
			++it;
		}
	}

	ImGui::Separator();
	ImGui::Spacing();

	// 「Add Component」ボタンを表示
	if(ImGui::Button("Add Component", ImVec2(-1, 30))) {
		ImGui::OpenPopup("AddComponentPopup");
	}

	// ボタンが押されたらポップアップメニューを開く
	if(ImGui::BeginPopup("AddComponentPopup")) {
		ComponentRegistry::DrawAddComponentMenu(this);
		ImGui::EndPopup();
	}
}

json GameObject::Serialize() const {
	json j;
	j["name"] = name_;

	// Transform情報の書き出し（ラジアンのまま直で保存！）
	j["transform"]["position"] = { transform_.translate.x, transform_.translate.y, transform_.translate.z };
	j["transform"]["rotation"] = { transform_.rotate.x, transform_.rotate.y, transform_.rotate.z };
	j["transform"]["scale"]    = { transform_.scale.x, transform_.scale.y, transform_.scale.z };

	// 各コンポーネントのSerializeを呼ぶ
	j["components"] = json::array();
	for (const auto& comp : components_) {
		json compJ;
		comp->Serialize(compJ);
		j["components"].push_back(compJ);
	}

	return j;
}

void GameObject::Deserialize(const json& j) {
	name_ = j["name"];

	// Transformの復元（ラジアンのまま直で復元！）
	if (j.contains("transform")) {
		auto pos = j["transform"]["position"];
		transform_.translate = { pos[0], pos[1], pos[2] };

		auto rot = j["transform"]["rotation"];
		transform_.rotate = { rot[0], rot[1], rot[2] };

		auto scale = j["transform"]["scale"];
		transform_.scale = { scale[0], scale[1], scale[2] };
	}

	// コンポーネントリストの復元
	if (j.contains("components")) {
		for (const auto& compJ : j["components"]) {
			ComponentRegistry::DeserializeComponent(this, compJ);
		}
	}
}

void GameObject::SetIsDebugMode(bool flag) {
	for (auto& comp : components_) {
		comp->SetIsDebugMode(flag);
	}
}
