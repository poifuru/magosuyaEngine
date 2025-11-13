#pragma once
#include <Wrl.h>
using namespace Microsoft::WRL;
#include <unordered_map>
#include <string>
#include <queue>
#include <vector>
#include "struct.h"

class ModelManager {
public:		//メンバ関数
	ModelManager ();
	~ModelManager ();

	void Initialize ();
	void LoadModelData ();
	void UnloadModelData ();

private:	//内部関数
	//マテリアルファイルの読み込み関数
	MaterialData LoadMaterialTemplateFile (const std::string& directoryPath, const std::string& filename);

	//ファイル読み込みの関数
	ModelData LoadObjFile (const std::string& directoryPath, const std::string& filename, bool inversion = false);

private:	//メンバ変数
	std::unordered_map<std::string, ModelData> map_;
};