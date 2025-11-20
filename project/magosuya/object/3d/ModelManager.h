#pragma once
#include <Windows.h>
#include <wrl.h>
using namespace Microsoft::WRL;
#include <unordered_map>
#include <string>
#include <queue>
#include <vector>
#include "struct.h"

class DxCommon;
class TextureManager;

class ModelManager {
public:		//メンバ関数
	static ModelManager* GetInstance () {
		//初めて呼び出されたときに一回だけ初期化
		static ModelManager instance;
		return &instance;
	}

	void Initialize (DxCommon* dxCommon, TextureManager* textureManager);

	ModelData* LoadModelData (const std::string& directoryPath, const std::string& id, bool inversion = false);
	std::weak_ptr<ModelData> GetModelData (std::string id);
	void UnloadModelData (const std::string& id);

private:
	//コンストラクタを禁止
	ModelManager () = default;
	// コピーコンストラクタと代入演算子を禁止
	ModelManager (const ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;
	ModelManager (ModelManager&&) = delete;
	ModelManager& operator=(ModelManager&&) = delete;

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