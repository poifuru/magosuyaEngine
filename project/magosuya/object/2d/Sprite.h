#pragma once    
#include <d3d12.h>  
#include <wrl.h>  
using namespace Microsoft::WRL;
#include <memory>
#include "SpriteRenderer.h"
#include "../../../general/struct.h"

class DxCommon; //前方宣言

class Sprite {  
	public:		//メンバ関数  
	Sprite (DxCommon* dxCommon);
	~Sprite ();

	void Initialize (Vector3 position, Vector2 size);
	void SetTexture (D3D12_GPU_DESCRIPTOR_HANDLE* handle);
	void MakewvpMatrix ();
	void Update (); 
	void Draw ();
	void ImGui ();

private:	//メンバ変数  
	//スプライトを構成するデータ
	TransformData transformData_;
	//使用するテクスチャ
	D3D12_GPU_DESCRIPTOR_HANDLE	 handle_;

	//描画を担当するレンダラークラス
	std::unique_ptr<SpriteRenderer> renderer_ = nullptr;

	//ポインタを借りてくる
	DxCommon* dxCommon_ = nullptr;
};