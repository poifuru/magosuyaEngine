#include "PCH.h"
#include "BoatWakeComponent.h"
#include "GameObject.h"
#include "BaseScene.h"
#include "GraphicsDevice.h"
#include "MathFunction.h"
#include "TextureManager.h"
#include "CameraOrganizer.h"
#include "ComponentType.h"
#include "MathFunction.h"

void BoatWakeComponent::Initialize() {
	if(isInitialized_) return;
	// 動的頂点バッファの初期化（UPLOADヒープで領域を確保）
	auto* graphicsDevice = GetGameObject()->GetContext()->graphicsDevice;
	auto* heapManager = GetGameObject()->GetContext()->heapManager;
	auto* shaderManager = GetGameObject()->GetContext()->shaderManager;
	auto* rootSigManager = GetGameObject()->GetContext()->rootSigManager;

	// RippleSimulator の初期化
	rippleSim_.Initialize(
		graphicsDevice->GetDevice(),
		*heapManager,
		*shaderManager,
		*rootSigManager,
		512, 512
	);

	// GenericMeshのバッファ領域をあらかじめ最大数で確保しておく
	std::vector<WakeVertex> dummyVerts(kMaxVertices);
	std::vector<uint32_t> dummyInds; // インデックスは使わないので空
	mesh_.Initialize(graphicsDevice->GetDevice(), dummyVerts, dummyInds);
	mesh_.inputLayout = MyEngine::Rendering::InputLayoutType::BoatWake; // もしくは専用レイアウト

	// 行列バッファの初期化
	transformBuffer_.Initialize(graphicsDevice->GetDevice());

	// マテリアルの初期設定
	material_ = std::make_unique<MyEngine::Rendering::Material>();
	material_->Initialize(graphicsDevice, heapManager);
	material_->SetShadingModel(MyEngine::Rendering::ShadingModel::BoatWake); // 航跡用シェーダー
	material_->SetBlendMode(MyEngine::Rendering::BlendModeType::Alpha);   // 半透明
	SetTexture(texPath_);

	if(gameObject_) {
		lastSpawnPosition_ = gameObject_->GetTransform().translate;
	}

	isInitialized_ = true;
}

void BoatWakeComponent::Update() {
	if(!gameObject_) return;

	// 寿命管理
	for(size_t i = 0; i < points_.size(); ) {
		points_[i].age += Time::GetDeltaTime();
		if(points_[i].age >= maxLifetime_) {
			points_.erase(points_.begin() + i);
		}
		else {
			++i;
		}
	}

	// 船のTransformから現在位置と右方向ベクトルを取得
	const auto& transform = gameObject_->GetTransform();
	Vector3 currentPos = transform.translate;

	// 回転行列から右方向(Right)ベクトルを算出 (スケール分は正規化)
	Matrix4x4 rotX = Math::MakeRotateXMatrix(transform.rotate.x);
	Matrix4x4 rotY = Math::MakeRotateYMatrix(transform.rotate.y);
	Matrix4x4 rotZ = Math::MakeRotateZMatrix(transform.rotate.z);
	Matrix4x4 matRotate = Math::Multiply(Math::Multiply(rotX, rotY), rotZ);

	Vector3 rightDir = { matRotate.m[0][0], matRotate.m[0][1], matRotate.m[0][2] };
	{
		float len = std::sqrt(rightDir.x * rightDir.x + rightDir.y * rightDir.y + rightDir.z * rightDir.z);
		if(len > 0.001f) {
			rightDir.x /= len; rightDir.y /= len; rightDir.z /= len;
		}
	}

	// 一定距離動いたら新しい制御点を追加
	Vector3 diff = { currentPos.x - lastSpawnPosition_.x, currentPos.y - lastSpawnPosition_.y, currentPos.z - lastSpawnPosition_.z };
	float distance = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

	if(distance >= spawnInterval_) {
		WakePoint newPt;
		newPt.position = currentPos;
		newPt.age = 0.0f;
		newPt.rightDir = rightDir;

		points_.insert(points_.begin(), newPt);
		lastSpawnPosition_ = currentPos;
	}

	// 行列バッファの更新 (ワールド行列は単位行列、WVPはViewProjectionをそのまま使う)
	CameraData& cameraData = CameraOrganizer::GetInstance()->GetCameraData();
	TransformMatrixData transformData{};
	transformData.World = Math::MakeIdentity4x4(); // 単位行列
	transformData.WVP = Math::Multiply(transformData.World, cameraData.vp); // ViewProjectionそのまま
	transformData.WorldInverseTranspose = Math::MakeIdentity4x4();
	transformBuffer_.Update(transformData);
}

void BoatWakeComponent::ImGui() {
	ImGui::Separator();
	ImGui::Text("Boat Wake Settings");
	ImGui::DragFloat("Spawn Interval", &spawnInterval_, 0.05f, 0.1f, 5.0f);
	ImGui::DragFloat("Max Lifetime", &maxLifetime_, 0.1f, 0.5f, 10.0f);
	ImGui::DragFloat("Trail Width", &trailWidth_, 0.1f, 0.1f, 10.0f);
	ImGui::DragFloat("Width Expand Rate", &widthExpandRate_, 0.1f, 0.0f, 5.0f);
}

void BoatWakeComponent::Serialize(json& j) const {
	j["type"] = "BoatWakeComponent";
	j["spawnInterval"] = spawnInterval_;
	j["maxLifetime"] = maxLifetime_;
	j["trailWidth"] = trailWidth_;
	j["widthExpandRate"] = widthExpandRate_;
}

void BoatWakeComponent::Deserialize(const json& j) {
	auto* device = GetGameObject()->GetContext()->graphicsDevice->GetDevice();

	Initialize();

	std::vector<WakeVertex> dummyVerts(kMaxVertices);
	std::vector<uint32_t> dummyInds;
	mesh_.Initialize(device, dummyVerts, dummyInds);

	if(j.contains("spawnInterval")) spawnInterval_ = j["spawnInterval"];
	if(j.contains("maxLifetime")) maxLifetime_ = j["maxLifetime"];
	if(j.contains("trailWidth")) trailWidth_ = j["trailWidth"];
	if(j.contains("widthExpandRate")) widthExpandRate_ = j["widthExpandRate"];

	isInitialized_ = true;
}

void BoatWakeComponent::DispatchCS(ID3D12GraphicsCommandList* cmdList) {
	if(!gameObject_) return;

	// シーンから水面を探して位置とサイズを取得
	Vector2 waterMin = Vector2(-999.0f, -999.0f);
	Vector2 waterSize = Vector2(100.0f, 100.0f);

	if(auto* waterComp = GetGameObject()->FindComponentInScene<WaterSurfaceComponent>()) {
		waterMin = waterComp->GetWaterMin();
		waterSize = waterComp->GetWaterSize();
	}

	const Vector3& boatPos = gameObject_->GetTransform().translate;

	// 前フレームからの移動距離（スピード）を計算
	float speed = Math::Length((boatPos - lastPos_));
	lastPos_ = boatPos; // 次回用に保存

	// プレイヤーの前方ベクトルを取得
	Vector2 forward = {};
	if(auto* playerComp = GetGameObject()->FindComponentInScene<PlayerComponent>()) {
		Vector3 forward3D = playerComp->GetForward();
		forward3D = Math::Normalize(forward3D);

		forward.x = forward3D.x;
		forward.y = forward3D.z;
	}

	// CSを実行してテクスチャを更新
	rippleSim_.Dispatch(cmdList, boatPos, speed, Time::GetDeltaTime(), waterMin, waterSize, forward);

	// CSが作ったテクスチャのインデックスをマテリアルへ指定
	if (material_) {
		material_->SetTextureIndex(rippleSim_.GetTextureSrvIndex());
	}
}

void BoatWakeComponent::SetTexture(const std::string& textureName) {
	GameObject* owner = GetGameObject();
	if(!owner) return;
	SceneContext* context = owner->GetContext();
	if(!context) return;

	texPath_ = textureName;

	// TextureManagerから登録されているテクスチャのインデックス（ハンドル）を取得する
	texIndex_ = context->textureManager->LoadTexture(texPath_);

	material_->SetTextureIndex(texIndex_);
}