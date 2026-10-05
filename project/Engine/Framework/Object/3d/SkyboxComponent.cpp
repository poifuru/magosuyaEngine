#include "PCH.h"
#include "SkyboxComponent.h"
#include "imgui.h"
#include "GameObject.h"
#include "BaseScene.h"
#include "TextureManager.h"
#include "RenderingModel.h"
#include "MathFunction.h"
#include "Material.h"
#include "CameraOrganizer.h"
#include "BaseCamera.h"
#include "GraphicsDevice.h"

namespace {
	const uint32_t kVertexNum = 8;
	const uint32_t kIndexNum = 36;

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBuffer(ID3D12Device* device, size_t size) {
		D3D12_HEAP_PROPERTIES uploadHeapProperties = {};
		uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = size;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		HRESULT hr = device->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&resource)
		);
		assert(SUCCEEDED(hr));
		return resource;
	}
}

SkyboxComponent::~SkyboxComponent() {
	if (vertexBuffer_ && vertexData_) {
		vertexBuffer_->Unmap(0, nullptr);
	}
	if (indexBuffer_ && indexData_) {
		indexBuffer_->Unmap(0, nullptr);
	}
}

void SkyboxComponent::Initialize() {
	if (isInitialized_) return;
	isInitialized_ = true;

	GameObject* owner = GetGameObject();
	if (!owner) return;
	SceneContext* context = owner->GetContext();
	if (!context) return;

	ID3D12Device* device = context->graphicsDevice->GetDevice();

	// デフォルトのスカイボックステクスチャを指定
	uint32_t textureIndex_ = 0;
	std::string texturePath_ = "Resources/Skybox/test2.dds";

	// === リソースの作成とマッピング ===
	// 頂点バッファ
	vertexBuffer_ = CreateBuffer(device, sizeof(SkyboxVertex) * kVertexNum);
	vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

	vbv_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vbv_.SizeInBytes = UINT(sizeof(SkyboxVertex) * kVertexNum);
	vbv_.StrideInBytes = sizeof(SkyboxVertex);

	// 実際の頂点データ（立方体の8角）
	vertexData_[0].position = { -1.0f,  1.0f,  1.0f, 1.0f };
	vertexData_[1].position = {  1.0f,  1.0f,  1.0f, 1.0f };
	vertexData_[2].position = { -1.0f, -1.0f,  1.0f, 1.0f };
	vertexData_[3].position = {  1.0f, -1.0f,  1.0f, 1.0f };
	vertexData_[4].position = { -1.0f,  1.0f, -1.0f, 1.0f };
	vertexData_[5].position = {  1.0f,  1.0f, -1.0f, 1.0f };
	vertexData_[6].position = { -1.0f, -1.0f, -1.0f, 1.0f };
	vertexData_[7].position = {  1.0f, -1.0f, -1.0f, 1.0f };

	// インデックスバッファ
	indexBuffer_ = CreateBuffer(device, sizeof(uint32_t) * kIndexNum);
	indexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));

	ibv_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
	ibv_.SizeInBytes = UINT(sizeof(uint32_t) * kIndexNum);
	ibv_.Format = DXGI_FORMAT_R32_UINT;

	// 実際のインデックスデータ
	indexData_[0] = 0; indexData_[1] = 1; indexData_[2] = 2; // 前
	indexData_[3] = 1; indexData_[4] = 3; indexData_[5] = 2;
	indexData_[6] = 5; indexData_[7] = 4; indexData_[8] = 7; // 後ろ
	indexData_[9] = 4; indexData_[10] = 6; indexData_[11] = 7;
	indexData_[12] = 1; indexData_[13] = 5; indexData_[14] = 3; // 右
	indexData_[15] = 5; indexData_[16] = 7; indexData_[17] = 3;
	indexData_[18] = 4; indexData_[19] = 0; indexData_[20] = 6; // 左
	indexData_[21] = 0; indexData_[22] = 2; indexData_[23] = 6;
	indexData_[24] = 4; indexData_[25] = 5; indexData_[26] = 0; // 上
	indexData_[27] = 5; indexData_[28] = 1; indexData_[29] = 0;
	indexData_[30] = 2; indexData_[31] = 3; indexData_[32] = 6; // 下
	indexData_[33] = 3; indexData_[34] = 7; indexData_[35] = 6;

	// トランスフォーム行列バッファ
	matrixBuffer_ = std::make_unique<ConstantBuffer<TransformMatrixData>>();
	matrixBuffer_->Initialize(device);

	mesh_.vbView = vbv_;
	mesh_.ibView = ibv_;
	mesh_.indexCount = kIndexNum;
	mesh_.inputLayout = MyEngine::Rendering::InputLayoutType::Skybox;

	// マテリアルの生成と設定
	material_ = std::make_shared<MyEngine::Rendering::Material>();
	material_->Initialize(context->graphicsDevice,
						  context->heapManager
	);

	material_->SetShadingModel(MyEngine::Rendering::ShadingModel::Skybox);
	material_->SetBlendMode(MyEngine::Rendering::BlendModeType::Opaque);
	material_->SetDoubleSided(true); // カリングを無効化（両面描画）
	material_->SetDepthWrite(false); // 深度の書き込みをしない
	material_->SetTextureIndex(textureIndex_);

	// デシリアライズ等でテクスチャパスが既に設定されていればロードする
	if (!texturePath_.empty()) {
		SetTexture(texturePath_);
	}

	// テクスチャパスが空ならデフォルトをセット
	if (texturePath_.empty()) {
		texturePath_ = "Resources/Skybox/test2.dds";
	}
	SetTexture(texturePath_);
}

void SkyboxComponent::Update() {
	if (!gameObject_) return;
	
	// マテリアルの更新情報をGPUへ転送
	if (material_) {
		material_->Update();
	}

	// カメラ座標を取得して追従させる
	CameraData& cameraData = CameraOrganizer::GetInstance()->GetCameraData();
	
	// スカイボックスは巨大な立方体として表現する
	Vector3 scale = { 500.0f, 500.0f, 500.0f };
	Vector3 rotate = { 0.0f, 0.0f, 0.0f };
	Vector3 translate = cameraData.transform.translate;

	TransformMatrixData transformMatrixData{};
	transformMatrixData.World = Math::MakeAffineMatrix(scale, rotate, translate);
	transformMatrixData.WVP = Math::Multiply(transformMatrixData.World, cameraData.vp);
	transformMatrixData.WorldInverseTranspose = Math::MakeIdentity4x4();

	matrixBuffer_->Update(transformMatrixData);
}

const MyEngine::Rendering::Mesh& SkyboxComponent::GetMesh() {
	return mesh_;
}

MyEngine::Rendering::Material* SkyboxComponent::GetMaterial() {
	return material_.get();
}

D3D12_GPU_VIRTUAL_ADDRESS SkyboxComponent::GetTransformAddress() {
	return matrixBuffer_->GetGPUVirtualAddress();
}

void SkyboxComponent::SetTexture(const std::string& filePath) {
	texturePath_ = filePath;
	GameObject* owner = GetGameObject();
	if (!owner) return;
	SceneContext* context = owner->GetContext();
	if (!context) return;

	textureIndex_ = context->textureManager->LoadTexture(filePath);

	if (material_) {
		material_->SetTextureIndex(textureIndex_);
	}
}

void SkyboxComponent::Serialize(json& j) const {
	j["type"] = "SkyboxComponent";
	j["texturePath"] = texturePath_;
}

void SkyboxComponent::Deserialize(const json& j) {
	if (j.contains("texturePath")) {
		texturePath_ = j["texturePath"];
	}
}

MyEngine::Rendering::InputLayoutType SkyboxComponent::GetInputLayoutType() {
	return MyEngine::Rendering::InputLayoutType::Skybox;
}

void SkyboxComponent::ImGui() {
	char texBuf[256];
	strcpy_s(texBuf, texturePath_.c_str());
	if (ImGui::InputText("Texture Path", texBuf, sizeof(texBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
		SetTexture(texBuf);
	}
	ImGui::Text("Loaded Index: %u", textureIndex_);
}
