#include "ModelManager.h"

MaterialData ModelManager::LoadMaterialTemplateFile (const std::string& directoryPath, const std::string& filename) {
	//必要な変数の宣言
	MaterialData materialData;
	std::string line;

	//ファイルを開く
	std::ifstream file (directoryPath + "/" + filename);
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

ModelData ModelManager::LoadObjFile (const std::string& directoryPath, const std::string& filename, bool inversion) {
	//必要になる変数宣言
	ModelData modelData;			//構築するModelData
	std::vector<Vector4> positions;	//位置
	std::vector<Vector3> normals;	//法線
	std::vector<Vector2> texcoords;	//テクスチャ座標
	std::string line;				//ファイルから読んだ１行を格納するもの

	//ファイルを開く
	std::ifstream file (directoryPath + "/" + filename + ".obj");	//ファイルを開く
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
			VertexData triangle[3];
			//面は三角形限定。その他は未対応
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefention;
				s >> vertexDefention;

				//頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分解してIndexを取得する
				std::istringstream v (vertexDefention);
				uint32_t elementIndeices[3];
				for (int32_t element = 0; element < 3; ++element) {
					std::string index;
					std::getline (v, index, '/');	//区切りでインデックスを読んでいく
					elementIndeices[element] = std::stoi (index);
				}
				//要素へのIndexから、実際の要素の値を取得して、頂点を構築する
				Vector4 position = positions[elementIndeices[0] - 1];
				Vector2 texcoord = texcoords[elementIndeices[1] - 1];
				Vector3 normal = normals[elementIndeices[2] - 1];
				//VertexData vertex = { position, texcoord, normal };
				//modelData.vertices.push_back(vertex);
				triangle[faceVertex] = { position, texcoord, normal };
			}
			//頂点を逆順で登録することで	、回り順を逆にする
			modelData.vertices.push_back (triangle[2]);
			modelData.vertices.push_back (triangle[1]);
			modelData.vertices.push_back (triangle[0]);
		}
		else if (identifier == "mtllib") {
			//materialTemplateLibraryファイルの名前を取得する
			std::string materialFilename;
			s >> materialFilename;
			//基本にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
			modelData.material = LoadMaterialTemplateFile (directoryPath, materialFilename);
		}
	}

	//頂点数を取得
	modelData.vertexCount = static_cast<uint32_t>(modelData.vertices.size ());

	//ModelDataを返す
	return modelData;
}
