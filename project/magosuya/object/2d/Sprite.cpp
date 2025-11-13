#include "Sprite.h"
#include <DirectXTex.h>
#include "function.h"
#include "MathFunction.h"
#include "MagosuyaEngine.h"

Sprite::Sprite (MagosuyaEngine* magosuya) {
	magosuya_ = magosuya;
	renderer_ = std::make_unique<SpriteRenderer> (magosuya);
	handle_ = {};
}

Sprite::~Sprite () {
}

void Sprite::Initialize (Vector3 position, Vector2 size) {
	//transformの初期化
	transformData_.transform.scale = { size.x, size.y, 1.0f };
	transformData_.transform.rotate = {0.0f, 0.0f, rotation_};
	transformData_.transform.translate = position;

	//uvTransformの初期化
	transformData_.uvTransform.scale = { 1.0f, 1.0f, 1.0f };
	transformData_.uvTransform.rotate = {};
	transformData_.uvTransform.translate = {};

	//wvpMatrixの初期化
	transformData_.wvpMatrix = MakeIdentity4x4 ();

	size_ = size;
	AdjustTextureSize ();

	renderer_->Initialize ();
}

void Sprite::SetTexture (D3D12_GPU_DESCRIPTOR_HANDLE handle) {
	handle_ = handle;
}

void Sprite::MakewvpMatrix () {
	Matrix4x4 world = MakeAffineMatrix (transformData_.transform.scale, transformData_.transform.rotate, transformData_.transform.translate);
	Matrix4x4 view = MakeIdentity4x4 ();
	Matrix4x4 proj = MakeOrthographicMatrix (0, 0, 1280.0f, 720.0f, 0, 100.0f);
	transformData_.wvpMatrix = Multiply (world, Multiply (view, proj));
}

void Sprite::Update () {
	MakewvpMatrix ();
	renderer_->Update (transformData_.wvpMatrix, transformData_.uvTransform,
					   anchorPoint_, isFlipX_, isFlipY_, id_, textureLeftTop_, textureSize_);
}

void Sprite::Draw () {
	renderer_->Draw (handle_);
}

void Sprite::ImGui () {
	renderer_->ImGui (transformData_.transform, transformData_.uvTransform);
}

void Sprite::AdjustTextureSize () {
	const DirectX::TexMetadata& metadata = magosuya_->GetMetaData (id_);

	textureSize_.x = static_cast<float>(metadata.width);
	textureSize_.y = static_cast<float>(metadata.height);
	//画像サイズをテクスチャサイズに合わせる
	size_ = textureSize_;
}
