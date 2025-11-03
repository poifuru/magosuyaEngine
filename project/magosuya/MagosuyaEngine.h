#pragma once
#include "engine/engineCore/DxCommon.h"
#include "utility/resouceManager/TextureManager.h"

class MagosuyaEngine {
public:		//メンバ関数
	//コンストラクタ、デストラクタ
	MagosuyaEngine ();
	~MagosuyaEngine ();

	void Initialize ();

	void BeginFrame ();
	void EndFrame ();

	void Finalize ();

	//アクセッサ
	DxCommon* GetDxCommon () { return dxCommon_.get (); }
	TextureManager* GetTextureManger () { return textureManager_.get (); }

private:	//内部関数
	

private:	//メンバ変数
	std::unique_ptr<DxCommon> dxCommon_;
	std::unique_ptr<TextureManager> textureManager_;
};