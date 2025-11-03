#pragma once
#include <d3d12.h>
#include <Wrl.h>
using namespace Microsoft::WRL;

class DxCommon; //前方宣言

class SpriteRenderer {
public:		//外部公開メソッド
	SpriteRenderer (DxCommon* dxCommon);
	~SpriteRenderer ();
};

