#include "ModelManager.h"
#include <sstream>
#include <map>
#include "MagosuyaEngine.h"
#include "engine/DxCommon.h"

ModelManager::ModelManager (DxCommon* dxCommon, TextureManager* textureManager) {
	dxCommon_ = dxCommon;
	textureManager_ = textureManager;
}

ModelManager::~ModelManager () {
}

ModelData* ModelManager::LoadModelData (const std::string& directoryPath, const std::string& id, bool inversion) {
	//IDのモデルをすでに読み込んでいたら
	if (map_.count (id)) {
		//既存データを取得
		std::shared_ptr<ModelData> existingData = map_.at (id);
		//存在していたら既存のデータを返す
		return existingData.get ();
	}

	//読み込んでいなければ新規読み込み
	ModelData cpuData = LoadObjFile (directoryPath, id, inversion);
	//shared_ptrに入れてGPUリソース作成の準備
	std::shared_ptr<ModelData> newData = std::make_shared<ModelData> (std::move(cpuData));

	//頂点バッファの生成と設定
	newData->vertexBuffer = dxCommon_->CreateBufferResource (sizeof (VertexData) * newData->vertexCount);
	//頂点バッファにデータを書き込む
	VertexData* vertexDataPtr = nullptr;
	newData->vertexBuffer->Map (0, nullptr, reinterpret_cast<void**>(&vertexDataPtr));
	//CPUメモリからGPUリソースへデータをコピー
	memcpy (vertexDataPtr, newData->vertices.data (), sizeof (VertexData) * newData->vertexCount);
	newData->vertexBuffer->Unmap (0, nullptr);

	//頂点バッファビューの設定
	newData->vbView.BufferLocation = newData->vertexBuffer->GetGPUVirtualAddress ();
	newData->vbView.SizeInBytes = UINT (sizeof (VertexData) * newData->vertexCount);
	newData->vbView.StrideInBytes = sizeof (VertexData);

	//インデックスバッファの生成と設定
	newData->indexBuffer = dxCommon_->CreateBufferResource (sizeof (uint32_t) * newData->indexCount);

	//インデックスバッファにデータを書き込む
	uint32_t* indexDataPtr = nullptr;
	newData->indexBuffer->Map (0, nullptr, reinterpret_cast<void**>(&indexDataPtr));
	//CPUメモリからGPUリソースへデータをコピー
	memcpy (indexDataPtr, newData->indices.data (), sizeof (uint32_t) * newData->indexCount);
	newData->indexBuffer->Unmap (0, nullptr);

	//インデックスバッファビューの設定
	newData->ibView.BufferLocation = newData->indexBuffer->GetGPUVirtualAddress ();
	newData->ibView.SizeInBytes = UINT (sizeof (uint32_t) * newData->indexCount);
	newData->ibView.Format = DXGI_FORMAT_R32_UINT;

	//mapに登録
	map_[id] = newData;

	// CPUメモリを解放（必要に応じて）
	newData->vertices.clear ();
	newData->indices.clear ();

	//データ提供
	return map_.at (id).get ();
}

std::weak_ptr<ModelData> ModelManager::GetModelData (std::string id) {
	//ID指定してmapから持ってくる
	assert (map_.count (id));
	return map_.at (id);
}

void ModelManager::UnloadModelData (const std::string& id) {
	//キャッシュマップからデータを削除
	map_.erase (id);
}

MaterialData ModelManager::LoadMaterialTemplateFile (const std::string& directoryPath, const std::string& id) {
	//必要な変数の宣言
	MaterialData materialData;
	std::string line;

	//ファイルを開く
	std::ifstream file (directoryPath + "/" + id);
	assert (file.is_open ());

	//実際にファイルを読み、MaterialDataを構築していく
	while (std::getline (file, line)) {
		std::string identifier;
		std::istringstream s (line);
		s >> identifier;

		//identfierに応じた処理
		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			//連結してファイルパスにする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}

	//MaterialDataを返す
	return materialData;
}

ModelData ModelManager::LoadObjFile (const std::string& directoryPath, const std::string& id, bool inversion) {
	//モデルに貼り付けてるテクスチャも一緒にロード
	textureManager_->LoadTexture (directoryPath + "/" + id + ".png", id);

	//必要になる変数宣言
	ModelData modelData;			//構築するModelData
	std::vector<Vector4> positions;	//位置
	std::vector<Vector3> normals;	//法線
	std::vector<Vector2> texcoords;	//テクスチャ座標
	std::string line;				//ファイルから読んだ１行を格納するもの
	// 複合インデックス文字列 ("v/vt/vn") をキーとし、ユニーク頂点リストのインデックスを値とする
	std::map<std::string, uint32_t> uniqueVertices;

	//ファイルを開く
	std::ifstream file (directoryPath + "/" + id + ".obj");	//ファイルを開く
	assert (file.is_open ());	//とりあえず開けなかったら止める

	//実際にファイルを読み、ModelDataを構築していく
	while (std::getline (file, line)) {
		std::string identifier;
		std::istringstream s (line);
		s >> identifier;	//先頭の識別子を読む

		//identifierに応じた処理
		if (identifier == "v") {
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.x *= -1.0f;
			if (inversion) {
				position.x *= -1.0f;
			}
			position.w = 1.0f;
			positions.push_back (position);
		}
		else if (identifier == "vt") {
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoord.y = 1.0f - texcoord.y;
			texcoords.push_back (texcoord);
		}
		else if (identifier == "vn") {
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normal.x *= -1.0f;
			if (inversion) {
				normal.x *= -1.0f;
			}
			normals.push_back (normal);
		}
		else if (identifier == "f") {
			//面は三角形限定。その他は未対応
			std::string vertexDefinitions[3];
			//3つの頂点すべてを faceVertex = 0, 1, 2 の順で読み込む
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				s >> vertexDefinitions[faceVertex];
			}

			for (int32_t faceVertex = 2; faceVertex > 0; --faceVertex) {
				std::string& vertexDefinition = vertexDefinitions[faceVertex];

				// この複合インデックスが既出かチェック
				if (uniqueVertices.count (vertexDefinition)) {
					// 既出の場合: ユニーク頂点リストの既存のインデックスを取得
					uint32_t index = uniqueVertices.at (vertexDefinition);
					// 取得したインデックスを indices 配列に追加
					modelData.indices.push_back (index);
				}
				else {
					// 未出の場合: 新しいユニーク頂点として登録
					//複合インデックスを分解して、個別のインデックスを取得 (ここまでは既存コードと同じ)
					std::istringstream v (vertexDefinition);
					uint32_t elementIndices[3];
					for (int32_t element = 0; element < 3; ++element) {
						std::string indexStr;
						std::getline (v, indexStr, '/');
						elementIndices[element] = std::stoi (indexStr);
					}

					//新しいVertexDataを構築
					Vector4 position = positions[elementIndices[0] - 1];
					Vector2 texcoord = texcoords[elementIndices[1] - 1];
					Vector3 normal = normals[elementIndices[2] - 1];
					VertexData newVertex = { position, texcoord, normal };

					//modelData.verticesに追加し、そのインデックスを記録
					uint32_t newIndex = static_cast<uint32_t>(modelData.vertices.size ());
					modelData.vertices.push_back (newVertex);

					//マップに登録
					uniqueVertices[vertexDefinition] = newIndex;

					//indices にも新しいインデックスを追加
					modelData.indices.push_back (newIndex);
				}
			}
		}
		else if (identifier == "mtllib") {
			//materialTemplateLibraryファイルの名前を取得する
			std::string materialFilename;
			s >> materialFilename;
			//基本にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
			modelData.material = LoadMaterialTemplateFile (directoryPath, materialFilename);
		}
	}

	// 頂点数を取得 (重複が排除されたユニークな数になっているはず)
	modelData.vertexCount = static_cast<uint32_t>(modelData.vertices.size ());

	// インデックス数を取得
	modelData.indexCount = static_cast<uint32_t>(modelData.indices.size ());

	// ModelDataを返す
	return modelData;
}
