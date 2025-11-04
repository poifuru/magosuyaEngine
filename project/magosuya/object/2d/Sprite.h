#pragma once    
#include <d3d12.h>  
#include <wrl.h>  
using namespace Microsoft::WRL;
#include "../../../general/struct.h"
#include "../../../magosuya/engine/engineCore/DxCommon.h"

class Sprite {  
	public:		//メンバ関数  
	Sprite (DxCommon* dxCommon);
	~Sprite ();

	void Initialize (Vector3 position, Vector2 size);  
	void Update ();  

	// ImGuiで編集する用
	void ShowImGuiEditor ();

private:	//メンバ変数  
	//スプライトのデータ
	SpriteData sprite_;  

	VertexData* vertexData_ = nullptr;
	uint32_t* indexData_ = nullptr;
	Matrix4x4* matrixData_ = nullptr;
	Material* materialData_ = nullptr;

	//ImGuiで色をいじる
	float color_[4];

	//ポインタを借りてくる
	DxCommon* dxCommon_ = nullptr;
};