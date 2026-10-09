#include "PCH.h"
#include "MeshRendererComponent.h"
#include "GameObject.h"
#include "BaseScene.h"       // SceneContextを使うため
#include "ModelFactory.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "CameraOrganizer.h" // カメラ情報を取得するため
#include "Model.h"

MeshRendererComponent::MeshRendererComponent() = default;

MeshRendererComponent::~MeshRendererComponent() = default;

void MeshRendererComponent::Initialize() {
	if (isInitialized_) return;
	isInitialized_ = true;

	// 初期モデルのロード
	SetModel(modelPath_);
	SetTexture(texPath_);
}
void MeshRendererComponent::Update() {
	if(!model_ || !gameObject_) return;

	// モデルにデプス設定を適用
	model_->SetDepthEnable(isDepthEnable_);

	// カメラのデータを取得してモデルをアップデート
	const CameraData& cameraData = CameraOrganizer::GetInstance()->GetCameraData();
	model_->Update(&cameraData);
}

void MeshRendererComponent::ImGui() {
	// モデルパスの表示と変更（簡易的なInputText）
	char pathBuf[256];
	strcpy_s(pathBuf, modelPath_.c_str());
	if (ImGui::InputText("Model Path", pathBuf, sizeof(pathBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
		SetModel(pathBuf);
	}
	ImGui::Spacing();

	// テクスチャパスの表示と変更
	char texBuf[256];
	strcpy_s(texBuf, texPath_.c_str());
	if (ImGui::InputText("Texture Path", texBuf, sizeof(texBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
		SetTexture(texBuf);
	}
	ImGui::Spacing();

	// デプスの有効・無効のチェックボックス
	if (ImGui::Checkbox("Depth Enable", &isDepthEnable_)) {
		if (model_) {
			model_->SetDepthEnable(isDepthEnable_);
		}
	}
	ImGui::Spacing();

	// 表示・非表示のチェックボックス
	ImGui::Checkbox("Visible", &isVisible_);
	ImGui::Spacing();

	// 既存のModelクラスが持っているImGuiの調整機能もそのまま呼べる
	if(model_) {
		model_->ImGui("Model Material");
	}
}

void MeshRendererComponent::Serialize(json& j) const {
	j["type"] = "MeshRendererComponent";
	j["modelPath"] = modelPath_;
	j["texPath"] = texPath_;
	j["isDepthEnable"] = isDepthEnable_;
	j["isVisible"] = isVisible_;

	if (model_) {
		if (auto material = model_->GetMaterial()) {
			const auto& matData = material->GetMaterialData();
			j["material"]["color"] = { matData.color.x, matData.color.y, matData.color.z, matData.color.w };
			j["material"]["roughness"] = matData.roughness;
			j["material"]["metallic"] = matData.metallic;
			j["material"]["environmentCoefficient"] = matData.environmentCoefficient;
			j["material"]["enableLighting"] = matData.enableLighting;
			j["material"]["time"] = matData.time;

			const auto& uv = material->GetUvTransform();
			j["material"]["uvTransform"]["scale"] = { uv.scale.x, uv.scale.y, uv.scale.z };
			j["material"]["uvTransform"]["rotation"] = { uv.rotate.x, uv.rotate.y, uv.rotate.z };
			j["material"]["uvTransform"]["translation"] = { uv.translate.x, uv.translate.y, uv.translate.z };
		}
	}
}
void MeshRendererComponent::Deserialize(const json& j) {
	isInitialized_ = true;
	if (j.contains("modelPath")) {
		SetModel(j["modelPath"]);
	}
	if (j.contains("texPath")) {
		SetTexture(j["texPath"]);
	}
	if (j.contains("isDepthEnable")) {
		isDepthEnable_ = j["isDepthEnable"];
	}
	if (j.contains("isVisible")) {
		isVisible_ = j["isVisible"];
	}

	if (model_ && j.contains("material")) {
		if (auto material = model_->GetMaterial()) {
			const auto& matJ = j["material"];
			if (matJ.contains("color")) {
				material->SetColor({ matJ["color"][0], matJ["color"][1], matJ["color"][2], matJ["color"][3] });
			}
			if (matJ.contains("roughness")) {
				material->SetRoughness(matJ["roughness"]);
			}
			if (matJ.contains("metallic")) {
				material->SetMetallic(matJ["metallic"]);
			}
			if (matJ.contains("environmentCoefficient")) {
				material->SetEnvironmentCoefficient(matJ["environmentCoefficient"]);
			}
			if (matJ.contains("enableLighting")) {
				material->SetEnableLighting(matJ["enableLighting"]);
			}
			if(matJ.contains("time")) {
				material->SetTime(matJ["time"]);
			}
			if (matJ.contains("uvTransform")) {
				const auto& uvJ = matJ["uvTransform"];
				EulerTransform uv;
				uv.scale = { uvJ["scale"][0], uvJ["scale"][1], uvJ["scale"][2] };
				uv.rotate = { uvJ["rotation"][0], uvJ["rotation"][1], uvJ["rotation"][2] };
				uv.translate = { uvJ["translation"][0], uvJ["translation"][1], uvJ["translation"][2] };
				material->SetUvTransform(uv);
			}
			material->Update();
		}
	}
}

void MeshRendererComponent::SetModel(const std::string& modelPath) {
	GameObject* owner = GetGameObject();
	if (!owner) return;
	SceneContext* context = owner->GetContext();
	if (!context) return;
	modelPath_ = modelPath;
	
	// モデルデータのロード
	modelIndex_ = context->modelManager->LoadModelData(modelPath_);
	
	// モデルの生成（一旦リセットしてから再生成）
	model_.reset();
	model_ = context->modelFactory->CreateModel(modelIndex_, texIndex_);

	if (model_ && gameObject_) {
		model_->SetParentTransform(&gameObject_->GetTransform());
	}
}

MyEngine::Rendering::Model* MeshRendererComponent::GetModel() const {
	return model_.get();
}

MyEngine::Rendering::Material* MeshRendererComponent::GetMaterial() const {
	return model_->GetMaterial();
}

void MeshRendererComponent::SetTexture(const std::string& textureName) {
	GameObject* owner = GetGameObject();
	if (!owner) return;
	SceneContext* context = owner->GetContext();
	if (!context) return;

	texPath_ = textureName;

	if (model_) {
		// TextureManagerから登録されているテクスチャのインデックス（ハンドル）を取得する
		texIndex_ = context->textureManager->LoadTexture(texPath_);

		// モデル（マテリアル）にインデックスを設定する
		model_->SetTextureIndex(texIndex_);
	}
}

void MeshRendererComponent::SetColor(const Vector4& color) {
	if (model_) {
		if (auto material = model_->GetMaterial()) {
			material->SetColor(color);
		}
	}
}

void MeshRendererComponent::SetEnableLighting(bool flag) {
	if (model_) {
		if (auto material = model_->GetMaterial()) {
			material->SetEnableLighting(flag ? TRUE : FALSE);
		}
	}
}

void MeshRendererComponent::SetBlendMode(MyEngine::Rendering::BlendModeType mode) {
	if (model_) {
		model_->SetBlendMode(mode);
	}
}

void MeshRendererComponent::SetDepthEnable(bool flag) {
	isDepthEnable_ = flag;
	if (model_) {
		model_->SetDepthEnable(flag);
	}
}

void MeshRendererComponent::SetLayer(uint8_t layer) {
	if (model_) {
		model_->SetLayer(layer);
	}
}

void MeshRendererComponent::SetDoubleSided(bool flag) {
	if (model_) {
		model_->SetDoubleSided(flag);
	}
}
