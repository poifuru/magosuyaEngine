#pragma once
#include <memory>
#include <string>
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

#pragma region 関数を呼び出しやすくする
	void LoadTexture (const std::string& filePath, const std::string& ID);
	void UnloadTexture (const std::string& filePath);
	D3D12_GPU_DESCRIPTOR_HANDLE* GetTextureHandle (const std::string& ID);
#pragma endregion

	//アクセッサ
	DxCommon* GetDxCommon () { return dxCommon_.get (); }
	TextureManager* GetTextureManger () { return textureManager_.get (); }

private:	//内部関数
	

private:	//メンバ変数
	std::unique_ptr<DxCommon> dxCommon_;
	std::unique_ptr<TextureManager> textureManager_;
};