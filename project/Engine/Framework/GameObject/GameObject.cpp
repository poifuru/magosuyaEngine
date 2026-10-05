#include "PCH.h"
#include "GameObject.h"
#include "Component.h"
#include "ComponentType.h"
#include "GraphicsDevice.h"
#include "CameraOrganizer.h"
#include "EditorManager.h"

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

void GameObject::Update() {
	for (auto& component : components_) {
		component->Update();
	}

	// アフィン行列作成
	Matrix4x4 world = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	
	// カメラデータを取得
	CameraData& cameraData = CameraOrganizer::GetInstance()->GetCameraData();

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
	CameraData& cameraData = CameraOrganizer::GetInstance()->GetCameraData();
	TransformMatrixData transformData{};
	transformData.World = world;
	transformData.WVP = Math::Multiply(world, cameraData.vp);
	transformData.WorldInverseTranspose = Math::MakeIdentity4x4();

	// バッファ更新
	transformBuffer_.Update(transformData);

	// --- 描画用コンポーネントのバッファも最新のカメラ行列で更新 ---
	if (auto* meshRenderer = GetComponent<MeshRendererComponent>()) {
		meshRenderer->Update();
	}
	if (auto* skybox = GetComponent<SkyboxComponent>()) {
		skybox->Update();
	}
	if (auto* sprite = GetComponent<SpriteComponent>()) {
		sprite->Update();
	}
	if (auto* water = GetComponent<WaterSurfaceComponent>()) {
		water->Update();
	}
}

void GameObject::ImGui() {
	// オブジェクト名の編集
	char nameBuf[128];
	strcpy_s(nameBuf, name_.c_str());
	if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
		name_ = nameBuf;
		std::string newName = nameBuf;
		// 空文字やスペース/タブだけの名前は反映させない（元の名前を維持する）
		if (!newName.empty() && newName.find_first_not_of(" \t\r\n") != std::string::npos) {
			name_ = newName;
		}
	}

	ImGui::Separator();

	// Transformの編集（デフォルトで常に表示）
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragFloat3("Position", &transform_.translate.x, 0.1f);
		ImGui::DragFloat3("Rotation", &transform_.rotate.x, 0.1f);
		ImGui::DragFloat3("Scale", &transform_.scale.x, 0.1f);
	}

	// 各コンポーネントのImGui描画（折りたたみヘッダーで表示）
	for (auto it = components_.begin(); it != components_.end(); ) {
		auto& component = *it;
		bool isDeleted = false;

		ImGui::PushID(component.get());
		bool open = false;
		// 2列のテーブルを作成（境界線なし、幅は自動引き伸ばし）
		if (ImGui::BeginTable("ComponentHeaderTable", 2, ImGuiTableFlags_NoSavedSettings)) {
			// 1列目（ヘッダー用）は引き伸ばし、2列目（ボタン用）は25ピクセル固定
			ImGui::TableSetupColumn("Header", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Button", ImGuiTableColumnFlags_WidthFixed, 25.0f);
			ImGui::TableNextRow();
			// 1列目に移動してヘッダーを描画
			ImGui::TableNextColumn();
			open = ImGui::CollapsingHeader(component->GetName(), ImGuiTreeNodeFlags_DefaultOpen);
			// 2列目に移動してXボタンを描画
			ImGui::TableNextColumn();
			if (ImGui::Button("X", ImVec2(20, 20))) {
				isDeleted = true;
			}
			ImGui::EndTable();
		}
		// ヘッダーの中身はテーブルの外側でウィンドウ幅いっぱいに描画する
		if (open) {
			component->ImGui();
		}
		ImGui::PopID();


		// 安全に要素を削除してループを回す
		if (isDeleted) {
			it = components_.erase(it);
		} else {
			++it;
		}
	}

	ImGui::Separator();
	ImGui::Spacing();

	// 「Add Component」ボタンを表示
	if (ImGui::Button("Add Component", ImVec2(-1, 30))) {
		ImGui::OpenPopup("AddComponentPopup");
	}

	// ボタンが押されたらポップアップメニューを開く
	if(ImGui::BeginPopup("AddComponentPopup")) {

		// Mesh Renderer の追加メニュー
		// MeshRendererComponent を持っていない場合だけ選択できるようにする
		if(GetComponent<MeshRendererComponent>() == nullptr) {
			if(ImGui::MenuItem("Mesh Renderer")) {
				// コンポーネントを追加して、すぐに初期化を呼ぶ
				auto* newComp = AddComponent<MeshRendererComponent>();
				newComp->Initialize();
			}
		}
		else {
			// すでに追加されている場合はグレーアウト表示
			ImGui::TextDisabled("Mesh Renderer (Already Added)");
		}

		// Light の追加メニュー
		if(GetComponent<LightComponent>() == nullptr) {
			if(ImGui::MenuItem("Light")) {
				auto* newComp = AddComponent<LightComponent>();
				newComp->Initialize();
			}
		}
		else {
			ImGui::TextDisabled("Light (Already Added)");
		}
		// 実体カメラ
		if(GetComponent<MainCameraComponent>() == nullptr) {
			if(ImGui::MenuItem("Main Camera")) {
				auto* newComp = AddComponent<MainCameraComponent>();
				newComp->Initialize();
			}
		}
		else {
			ImGui::TextDisabled("Main Camera (Already Added)");
		}
		// 仮想デバッグカメラ
		if(GetComponent<VirtualDebugCamera>() == nullptr) {
			if(ImGui::MenuItem("Virtual Debug Camera")) {
				auto* newComp = AddComponent<VirtualDebugCamera>();
				newComp->Initialize();
			}
		}
		else {
			ImGui::TextDisabled("Debug Camera (Already Added)");
		}
		// 仮想追従カメラ
		if(GetComponent<VirtualFollowCamera>() == nullptr) {
			if(ImGui::MenuItem("Virtual Follow Camera")) {
				auto* newComp = AddComponent<VirtualFollowCamera>();
				newComp->Initialize();
			}
		}
		else {
			ImGui::TextDisabled("Follow Camera (Already Added)");
		}
		// プレイヤーコンポーネント
		if(GetComponent<PlayerComponent>() == nullptr) {
			if(ImGui::MenuItem("Player Component")) {
				auto* newComp = AddComponent<PlayerComponent>();
				newComp->Initialize();
			}
		}
		else {
			ImGui::TextDisabled("Player Component (Already Added)");
		}
		// レティクルコンポーネント
		if(GetComponent<ReticleComponent>() == nullptr) {
			if(ImGui::MenuItem("Reticle Component")) {
				auto* newComp = AddComponent<ReticleComponent>();
				newComp->Initialize();
			}
		}
		else {
			ImGui::TextDisabled("Reticle Component (Already Added)");
		}
		// コライダーコンポーネント
		if(GetComponent<ColliderComponent>() == nullptr) {
			if(ImGui::MenuItem("Collider Component")) {
				auto* newComp = AddComponent<ColliderComponent>();
				newComp->Initialize();
			}
		}
		else {
			ImGui::TextDisabled("Collider Component (Already Added)");
		}
		// 鳥の敵コンポーネント
		if(GetComponent<BirdEnemyComponent>() == nullptr) {
			if(ImGui::MenuItem("Bird Enemy Component")) {
				auto* newComp = AddComponent<BirdEnemyComponent>();
				newComp->Initialize();
			}
		}
		else {
			ImGui::TextDisabled("Bird Enemy Component (Already Added)");
		}
		// 魚の敵コンポーネント
		if(GetComponent<FishEnemyComponent>() == nullptr) {
			if(ImGui::MenuItem("Fish Enemy Component")) {
				auto* newComp = AddComponent<FishEnemyComponent>();
				newComp->Initialize();
			}
		}
		else {
			ImGui::TextDisabled("Fish Enemy Component (Already Added)");
		}
		// エネミーマネージャーコンポーネント
		if(GetComponent<EnemyManagerComponent>() == nullptr) {
			if(ImGui::MenuItem("Enemy Manager Component")) {
				auto* newComp = AddComponent<EnemyManagerComponent>();
				newComp->Initialize();
			}
		}
		else {
			ImGui::TextDisabled("Enemy Manager Component (Already Added)");
		}
		// スカイボックスコンポーネント
		if(GetComponent<SkyboxComponent>() == nullptr) {
			if(ImGui::MenuItem("Skybox Component")) {
				auto* newComp = AddComponent<SkyboxComponent>();
				newComp->Initialize();
			}
		}
		else {
			ImGui::TextDisabled("Skybox Component (Already Added)");
		}
		// スプライトコンポーネント
		if(GetComponent<SpriteComponent>() == nullptr) {
			if(ImGui::MenuItem("Sprite Component")) {
				auto* newComp = AddComponent<SpriteComponent>();
				newComp->Initialize();
			}
		}
		else {
			ImGui::TextDisabled("Sprite Component (Already Added)");
		}
		// ナンバー表示コンポーネント
		if(GetComponent<NumberDrawerComponent>() == nullptr) {
			if(ImGui::MenuItem("Number Drawer Component")) {
				auto* newComp = AddComponent<NumberDrawerComponent>();
				newComp->Initialize();
			}
		}
		else {
			ImGui::TextDisabled("Number Drawer Component (Already Added)");
		}
		// ゲームディレクターコンポーネント
		if(GetComponent<GameDirectorComponent>() == nullptr) {
			if(ImGui::MenuItem("Game Director Component")) {
				auto* newComp = AddComponent<GameDirectorComponent>();
				newComp->Initialize();
			}
		}
		else {
			ImGui::TextDisabled("Game Director Component (Already Added)");
		}
		// 水面コンポーネント
		if(GetComponent<WaterSurfaceComponent>() == nullptr) {
			if(ImGui::MenuItem("Water Surface Component")) {
				auto* newComp = AddComponent<WaterSurfaceComponent>();
				newComp->Initialize();
			}
		}
		else {
			ImGui::TextDisabled("Water Surface Component (Already Added)");
		}
		// 航跡波コンポーネント
		if(GetComponent<BoatWakeComponent>() == nullptr) {
			if(ImGui::MenuItem("Boat Wake Component")) {
				auto* newComp = AddComponent<BoatWakeComponent>();
				newComp->Initialize();
			}
		}
		else {
			ImGui::TextDisabled("Boat Wake Component (Already Added)");
		}
		// テキスト表示コンポーネント
		if(GetComponent<TextDrawerComponent>() == nullptr) {
			if(ImGui::MenuItem("Text Drawer Component")) {
				auto* newComp = AddComponent<TextDrawerComponent>();
				newComp->Initialize();
			}
		}
		else {
			ImGui::TextDisabled("Text Drawer Component (Already Added)");
		}

		// コンポーネントが増えたらここに
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
	if(j.contains("components")) {
		for(const auto& compJ : j["components"]) {
			// null などのオブジェクトじゃないものや、"type" キーが無いものは安全に無視する
			if (!compJ.is_object() || !compJ.contains("type")) {
				continue;
			}

			std::string type = compJ["type"];

			if(type == "MeshRendererComponent") {
				// 二重追加を防ぐチェックをしてから追加
				auto* comp = GetComponent<MeshRendererComponent>();
				if(!comp) comp = AddComponent<MeshRendererComponent>();
				comp->Deserialize(compJ);
			}
			else if(type == "LightComponent") {
				auto* comp = GetComponent<LightComponent>();
				if(!comp) comp = AddComponent<LightComponent>();
					comp->Deserialize(compJ);
			}
			else if(type == "MainCameraComponent") {
				auto* comp = GetComponent<MainCameraComponent>();
				if(!comp) comp = AddComponent<MainCameraComponent>();
				comp->Deserialize(compJ);
			}
			else if(type == "VirtualFollowCamera") {
				auto* comp = GetComponent<VirtualFollowCamera>();
				if(!comp) comp = AddComponent<VirtualFollowCamera>();
				comp->Deserialize(compJ);
			}
			else if (type == "VirtualDebugCamera") {
				auto* comp = GetComponent<VirtualDebugCamera>();
				if (!comp) comp = AddComponent<VirtualDebugCamera>();
				comp->Deserialize(compJ);
			}
			else if(type == "PlayerComponent") {
				auto* comp = GetComponent<PlayerComponent>();
				if(!comp) comp = AddComponent<PlayerComponent>();
				comp->Deserialize(compJ);
			}
			else if(type == "ReticleComponent") {
				auto* comp = GetComponent<ReticleComponent>();
				if(!comp) comp = AddComponent<ReticleComponent>();
				comp->Deserialize(compJ);
			}
			else if(type == "ColliderComponent") {
				auto* comp = GetComponent<ColliderComponent>();
				if(!comp) comp = AddComponent<ColliderComponent>();
				comp->Deserialize(compJ);
			}
			else if(type == "BirdEnemyComponent") {
				auto* comp = GetComponent<BirdEnemyComponent>();
				if(!comp) comp = AddComponent<BirdEnemyComponent>();
				comp->Deserialize(compJ);
			}
			else if(type == "FishEnemyComponent") {
				auto* comp = GetComponent<FishEnemyComponent>();
				if(!comp) comp = AddComponent<FishEnemyComponent>();
				comp->Deserialize(compJ);
			}
			else if(type == "EnemyManagerComponent") {
				auto* comp = GetComponent<EnemyManagerComponent>();
				if(!comp) comp = AddComponent<EnemyManagerComponent>();
				comp->Deserialize(compJ);
			}
			else if(type == "SkyboxComponent") {
				auto* comp = GetComponent<SkyboxComponent>();
				if(!comp) comp = AddComponent<SkyboxComponent>();
				comp->Deserialize(compJ);
			}
			else if(type == "SpriteComponent") {
				auto* comp = GetComponent<SpriteComponent>();
				if(!comp) comp = AddComponent<SpriteComponent>();
				comp->Deserialize(compJ);
			}
			else if(type == "NumberDrawerComponent") {
				auto* comp = GetComponent<NumberDrawerComponent>();
				if(!comp) comp = AddComponent<NumberDrawerComponent>();
				comp->Deserialize(compJ);
			}
			else if(type == "GameDirectorComponent") {
				auto* comp = GetComponent<GameDirectorComponent>();
				if(!comp) comp = AddComponent<GameDirectorComponent>();
				comp->Deserialize(compJ);
			}
			else if(type == "WaterSurfaceComponent") {
				auto* comp = GetComponent<WaterSurfaceComponent>();
				if(!comp) comp = AddComponent<WaterSurfaceComponent>();
				comp->Deserialize(compJ);
			}
			else if(type == "BoatWakeComponent") {
				auto* comp = GetComponent<BoatWakeComponent>();
				if(!comp) comp = AddComponent<BoatWakeComponent>();
				comp->Deserialize(compJ);
			}
			else if(type == "TextDrawerComponent") {
				auto* comp = GetComponent<TextDrawerComponent>();
				if(!comp) comp = AddComponent<TextDrawerComponent>();
				comp->Deserialize(compJ);
			}
		}
	}
}

void GameObject::SetIsDebugMode(bool flag) {
	for (auto& comp : components_) {
		comp->SetIsDebugMode(flag);
	}
}
