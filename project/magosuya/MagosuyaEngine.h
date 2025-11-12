#pragma once
#include <memory>
#include <string>
#include "engine/engineCore/DxCommon.h"
#include "utility/manager/TextureManager.h"
#include "utility/ImGuiManager.h"

class MagosuyaEngine {
public:		//メンバ関数
	//コンストラクタ、デストラクタ
	MagosuyaEngine ();
	~MagosuyaEngine ();

	void Initialize ();
	void BeginFrame ();
	void EndFrame ();
	void Finalize ();

#pragma region 便利関数
	void LoadTexture (const std::string& filePath, const std::string& ID);
	D3D12_GPU_DESCRIPTOR_HANDLE* GetTextureHandle (const std::string& ID);
#pragma endregion

	//アクセッサ
	DxCommon* GetDxCommon () { return dxCommon_.get (); }
	TextureManager* GetTextureManger () { return textureManager_.get (); }

private:	//内部関数
	

private:	//メンバ変数
	std::unique_ptr<DxCommon> dxCommon_;
	std::unique_ptr<TextureManager> textureManager_;
	std::unique_ptr<ImGuiManager> imGuiManager_;
};