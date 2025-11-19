#pragma once
#include <memory>
#include <string>
#include "engine/DxCommon.h"
#include "TextureManager.h"
#include "ImGuiManager.h"
#include "ModelManager.h"

class MagosuyaEngine {
public:		//メンバ関数
	//コンストラクタ、デストラクタ
	MagosuyaEngine ();
	~MagosuyaEngine ();

	void Initialize ();
	void BeginFrame ();
	void EndFrame ();
	void Finalize ();

#pragma region メンバ変数を便利に
	void LoadTexture (const std::string& filePath, const std::string& ID);
	void UnloadTexture (const std::string& filePath);
	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle (const std::string& ID);
	const DirectX::TexMetadata& GetMetaData (const std::string& id);
	void LoadModelData (const std::string& filePath, const std::string& ID, bool inversion = false);
	void UnloadModelData (const std::string& ID);
	std::weak_ptr<ModelData> GetModelData (const std::string& ID);
#pragma endregion

	//アクセッサ
	DxCommon* GetDxCommon () { return dxCommon_.get (); }

private:	//メンバ変数
	std::unique_ptr<DxCommon> dxCommon_;
	std::unique_ptr<TextureManager> textureManager_;
	std::unique_ptr<ImGuiManager> imGuiManager_;
	std::unique_ptr<ModelManager> modelManager_;
};