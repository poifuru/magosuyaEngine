#pragma once    
#include <d3d12.h>  
#include <wrl.h>  
using namespace Microsoft::WRL;
#include <memory>
#include "SpriteRenderer.h"
#include "struct.h"

//前方宣言
class MagosuyaEngine;

class Sprite {  
	public:		//メンバ関数  
	Sprite (MagosuyaEngine* magosuya);
	~Sprite ();

	void Initialize (Vector3 position, Vector2 size);
	void SetTexture (D3D12_GPU_DESCRIPTOR_HANDLE handle);
	void MakewvpMatrix ();
	void Update (); 
	void Draw ();
	void ImGui ();

private:	//プライベート関数
	void AdjustTextureSize ();

public:		//アクセッサ
	//位置
	Vector3 GetPosition () { return transformData_.transform.translate; }
	void SetPosition (Vector3& position) { transformData_.transform.translate = position; }
	//回転
	float GetRotation () { return rotation_; }
	void SetRotation (float rotation) { rotation_ = rotation; }
	//色
	const Vector4& GetColor () const { return renderer_->GetMaterial ()->color; }
	void SetColor (const Vector4& color) { renderer_->SetColor (color); }
	//サイズ
	const Vector2& GetSize ()const { return size_; }
	void SetSize (const Vector2& size) { size_ = size; }
	//アンカーポイント
	const Vector2& GetAnchorPoint ()const { return anchorPoint_; }
	void SetAnchorPoint (const Vector2& anchorPoint) { anchorPoint_ = anchorPoint; }
	//フリップ
	const bool& GetIsFlipX ()const { return isFlipX_; }
	void SetIsFlipX (const bool& flag) { isFlipX_ = flag; }
	const bool& GetIsFlipY ()const { return isFlipY_; }
	void SetIsFlipY (const bool& flag) { isFlipY_ = flag; }
	//テクスチャ左上座標
	const Vector2& GetTextureLeftTop ()const { return textureLeftTop_; }
	void SetTextureLeftTop (const Vector2& leftTop) { textureLeftTop_ = leftTop; }
	//テクスチャ切りだしサイズ
	const Vector2& GetTextureSize ()const { return textureSize_; }
	void SetTextureSize (const Vector2& size) { textureSize_ = size; }
	//id
	const std::string& GetID ()const { return id_; }
	void SetID (const std::string& id) { id_ = id; }

private:	//メンバ変数  
	//スプライトを構成するデータ
	TransformData transformData_ = {};
	//回転のデータ(Z軸だけなのでfloat、単位はradian)
	float rotation_ = 0.0f;
	//サイズ
	Vector2 size_ = {};
	//アンカーポイント
	Vector2 anchorPoint_ = {};
	//フリップのフラグ
	bool isFlipX_ = false;
	bool isFlipY_ = false;
	//テクスチャの左上座標
	Vector2 textureLeftTop_ = {};
	//テクスチャ切り出しサイズ
	Vector2 textureSize_ = {};

	//使用するテクスチャ
	D3D12_GPU_DESCRIPTOR_HANDLE	 handle_;
	//画像検索用のID
	std::string id_;

	//描画を担当するレンダラークラス
	std::unique_ptr<SpriteRenderer> renderer_ = nullptr;

	//ポインタを借りてくる
	MagosuyaEngine* magosuya_ = nullptr;
};