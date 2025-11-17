#pragma once
#include <memory>
#include <string>
#include "engine/DxCommon.h"
#include "utility/manager/TextureManager.h"
#include "utility/manager/ImGuiManager.h"
#include "utility/manager/ModelManager.h"
#include "utility/input/InputManager.h"

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
	//テクスチャマネージャー
	void LoadTexture (const std::string& filePath, const std::string& ID);
	void UnloadTexture (const std::string& filePath);
	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle (const std::string& ID);
	const DirectX::TexMetadata& GetMetaData (const std::string& id);

	//モデルマネージャー
	void LoadModelData (const std::string& filePath, const std::string& ID, bool inversion = false);
	void UnloadModelData (const std::string& ID);
	std::weak_ptr<ModelData> GetModelData (const std::string& ID);

	//インプットマネージャー
	RawInput* GetRawInput ();
	GamePad* GetGamePad ();
#pragma endregion

	//アクセッサ
	DxCommon* GetDxCommon () { return dxCommon_.get (); }

private:	//メンバ変数
	std::unique_ptr<DxCommon> dxCommon_;
	std::unique_ptr<TextureManager> textureManager_;
	std::unique_ptr<ImGuiManager> imGuiManager_;
	std::unique_ptr<ModelManager> modelManager_;
	std::unique_ptr<InputManager> inputManager_;
};