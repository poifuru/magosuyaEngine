#pragma once
#include <Wrl.h>
using namespace Microsoft::WRL;
#include <unordered_map>
#include <string>
#include <queue>
#include <vector>
#include "struct.h"
#include "TextureManager.h"

class DxCommon;

class ModelManager {
public:		//メンバ関数
	ModelManager (DxCommon* dxCommon, TextureManager* textureManager);
	~ModelManager ();

	ModelData* LoadModelData (const std::string& directoryPath, const std::string& id, bool inversion = false);
	std::weak_ptr<ModelData> GetModelData (std::string id);
	void UnloadModelData (const std::string& id);

private:	//内部関数
	//マテリアルファイルの読み込み関数
	MaterialFile LoadMaterialTemplateFile (const std::string& directoryPath, const std::string& id);

	//ファイル読み込みの関数
	ModelData LoadObjFile (const std::string& directoryPath, const std::string& id, bool inversion = false);

private:	//メンバ変数
	std::unordered_map<std::string, std::shared_ptr<ModelData>> map_;

	//ポインタを借りる
	DxCommon* dxCommon_ = nullptr;
	TextureManager* textureManager_ = nullptr;
};