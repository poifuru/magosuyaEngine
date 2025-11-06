#include "Sprite.h"
#include "../../../general/function.h"
#include "../../../general/Math.h"

Sprite::Sprite (DxCommon* dxCommon) {
	dxCommon_ = dxCommon;
	renderer_ = std::make_unique<SpriteRenderer> (dxCommon);
}

Sprite::~Sprite () {
}

void Sprite::Initialize (Vector3 position, Vector2 size) {
	//transformの初期化
	transformData_.transform.scale = { 1.0f, 1.0f, 1.0f };
	transformData_.transform.rotate = {};
	transformData_.transform.translate = position;

	//uvTransformの初期化
	transformData_.uvTransform.scale = { 1.0f, 1.0f, 1.0f };
	transformData_.uvTransform.rotate = {};
	transformData_.uvTransform.translate = {};

	//wvpMatrixの初期化
	transformData_.wvpMatrix = MakeIdentity4x4 ();

	renderer_->Initialize (size);
}

void Sprite::SetTexture (D3D12_GPU_DESCRIPTOR_HANDLE* handle) {
	handle_ = *handle;
}

void Sprite::MakewvpMatrix () {
	Matrix4x4 world = MakeAffineMatrix (transformData_.transform.scale, transformData_.transform.rotate, transformData_.transform.translate);
	transformData_.wvpMatrix = world;
}

void Sprite::Update () {
	MakewvpMatrix ();
	renderer_->Update (transformData_.wvpMatrix, transformData_.uvTransform);
}

void Sprite::Draw () {
	renderer_->Draw (handle_);
}

void Sprite::ImGui () {
	renderer_->ImGui (transformData_.transform, transformData_.uvTransform);
}
