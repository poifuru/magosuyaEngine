#include "Model.h"
#include <imgui.h>
#include "function.h"
#include "MathFunction.h"
#include "TextureManager.h"
#include "ModelManager.h"

Model::Model (DxCommon* dxCommon, TextureManager* texManager, ModelManager* modelManager) {
	texManager_ = texManager;
	modelManager_ = modelManager;
	renderer_ = std::make_unique<ModelRenderer> (dxCommon);
}

Model::~Model () {
}

void Model::Initialize (Vector3 scale, Vector3 rotate, Vector3 position) {
	transform_ = { scale, rotate, position };
	uvTransform_ = { { 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 0.0f },{ 0.0f, 0.0f, 0.0f } };
	renderer_->Initialize ();
}

void Model::Update (Matrix4x4* vp) {
	Matrix4x4 world = MakeAffineMatrix (transform_.scale, transform_.rotate, transform_.translate);

	renderer_->Update (world, *vp, uvTransform_);
}

void Model::Draw () {
	renderer_->Draw (texture_);
}

void Model::ImGui () {
	renderer_->ImGui (transform_, uvTransform_);
}

void Model::SetModelData (const std::string& ID) {
	modelData_ = modelManager_->GetModelData (ID);
	renderer_->SetModelData (modelData_);
	renderer_->SetImGuiID (ID); 
}

void Model::SetTexture (const std::string& ID) {
	texture_ = texManager_->GetTextureHandle (ID);
}