#include "PCH.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "MathFunction.h"
#include "MeshData.h"
#include "Material.h"
#include "RenderingModel.h"

namespace fs = std::filesystem;

void ModelManager::Initialize(ID3D12Device* device, TextureManager* textureManager) {
	device_ = device;
	textureManager_ = textureManager;
}

uint32_t ModelManager::LoadModelData(const std::string& filePath, bool inversion) {
	std::string key = MakeKey(filePath, inversion);

	// すでに読み込まれていたら既存データを返す
	if(modelPathToIndexMap_.count(key)) {
		return modelPathToIndexMap_.at(key);
	}

	// 新規読み込み
	MyEngine::Rendering::ModelData cpuData = LoadModelFile(filePath, inversion);

	// ロードしたテクスチャのインデックスを保存する領域を確保
	cpuData.defaultTextureIndices.resize(cpuData.meshes.size());
	
	// 頂点バッファ・インデックスバッファの生成と設定、及びテクスチャ自動ロード
	for (size_t i = 0; i < cpuData.meshes.size(); ++i) {
		auto& mesh = cpuData.meshes[i];

		mesh.Initialize(device_, mesh.vertices, mesh.indices);

		// i番目のメッシュに対応するテクスチャパスを取り出してロードする
		std::string texPath = cpuData.defaultTexturePaths[i];
		if (!texPath.empty()) {
			cpuData.defaultTextureIndices[i] = textureManager_->LoadTexture(texPath);
		} else {
			cpuData.defaultTextureIndices[i] = textureManager_->LoadTexture("white1x1");
		}
	}

	std::shared_ptr<MyEngine::Rendering::ModelData> newData = std::make_shared<MyEngine::Rendering::ModelData>(std::move(cpuData));

	uint32_t index = static_cast<uint32_t>(models_.size());
	models_.push_back(newData);
	modelPathToIndexMap_[key] = index;

	return index;
}

std::weak_ptr<MyEngine::Rendering::ModelData> ModelManager::GetModelData(uint32_t index) {
	assert(index < models_.size());
	return models_[index];
}

void ModelManager::UnloadModelData(const std::string& filePath, bool inversion) {
	std::string key = MakeKey(filePath, inversion);
	auto it = modelPathToIndexMap_.find(key);

	if (it != modelPathToIndexMap_.end()) {
		uint32_t index = it->second;
		models_[index].reset();         // モデルのデータを削除
		modelPathToIndexMap_.erase(it); // マップからも完全に消去
	}
}

Animation* ModelManager::LoadAnimationData(const std::string& directoryPath, const std::string& fileName) {
	if(animationMap_.count(fileName)) {
		return animationMap_.at(fileName).get();
	}

	Animation loadData = LoadAnimation(directoryPath, fileName);
	std::shared_ptr<Animation> newData = std::make_shared<Animation>(std::move(loadData));
	animationMap_[fileName] = newData;

	return animationMap_.at(fileName).get();
}

std::weak_ptr<Animation> ModelManager::GetAnimationData(std::string id) {
	assert(animationMap_.count(id));
	return animationMap_.at(id);
}

void ModelManager::UnloadAnimationData(const std::string& id) {
	animationMap_.erase(id);
}

MyEngine::Rendering::MaterialTex ModelManager::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& id) {
	MyEngine::Rendering::MaterialTex materialData;
	std::string line;

	std::ifstream file(directoryPath + "/" + id);
	assert(file.is_open());

	while(std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		if(identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			materialData.filePath = directoryPath + "/" + textureFilename;
		}
	}

	return materialData;
}

MyEngine::Rendering::ModelData ModelManager::LoadModelFile(const std::string& filePath, bool inversion) {
	// ★【調査用に追加】今どのファイルをロードしようとしているか出力する！
	OutputDebugStringA(("Loading Model: " + filePath + "\n").c_str());

	MyEngine::Rendering::ModelData modelData;
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(
		filePath.c_str(),
		aiProcess_FlipWindingOrder |
		aiProcess_FlipUVs |
		aiProcess_JoinIdenticalVertices |
		aiProcess_Triangulate |
		aiProcess_GenSmoothNormals
	);
	assert(scene && scene->HasMeshes());

	// ファイルパスからディレクトリパスを抽出
	std::string directoryPath = std::filesystem::path(filePath).parent_path().string();

	for(uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		aiMesh* mesh = scene->mMeshes[meshIndex];

		MyEngine::Rendering::StaticMesh myMesh;

		for(uint32_t i = 0; i < mesh->mNumVertices; ++i) {
			aiVector3D& position = mesh->mVertices[i];
			aiVector3D normal = mesh->HasNormals() ? mesh->mNormals[i] : aiVector3D(0.0f, 1.0f, 0.0f);
			aiVector3D texcoord = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][i] : aiVector3D(0.0f, 0.0f, 0.0f);

			VertexData vertex = {};
			if(inversion) {
				vertex.position = { position.x, position.y, position.z, 1.0f };
				vertex.normal = { normal.x, normal.y, normal.z };
			}
			else {
				vertex.position = { -position.x, position.y, position.z, 1.0f };
				vertex.normal = { -normal.x, normal.y, normal.z };
			}
			vertex.texcoord = { texcoord.x, texcoord.y };
			myMesh.vertices.push_back(vertex); 
		}

		for(uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			aiFace& face = mesh->mFaces[faceIndex];
			if (face.mNumIndices != 3) {
				continue;
			}

			for(uint32_t i = 0; i < face.mNumIndices; ++i) {
				myMesh.indices.push_back(face.mIndices[i]);
			}
		}

		if (myMesh.vertices.empty() || myMesh.indices.empty()) {
			continue;
		}

		// ボーンデータ(スキニング)があるかどうかは aiMesh::mNumBones で判定できる
		myMesh.inputLayout = (mesh->mNumBones > 0)
			? MyEngine::Rendering::InputLayoutType::SkinningStandard3D
			: MyEngine::Rendering::InputLayoutType::Standard3D;

		// このメッシュに適用されているマテリアルのテクスチャパスを取得
		std::string texPath = "";
		if(scene->mNumMaterials > 0 && mesh->mMaterialIndex < scene->mNumMaterials) {
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			if(material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
				aiString textureFilePath;
				material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
				texPath = directoryPath + "/" + textureFilePath.C_Str();
			}
		}
		modelData.defaultTexturePaths.push_back(texPath); 
		modelData.meshes.push_back(std::move(myMesh));
	}

	for(uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		aiMesh* mesh = scene->mMeshes[meshIndex];
		for(uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
			aiBone* bone = mesh->mBones[boneIndex];
			std::string jointName = bone->mName.C_Str();
			MyEngine::Rendering::JointWeightData& jointWeightData = modelData.skinClusterData[jointName];

			const aiMatrix4x4& ai = bone->mOffsetMatrix;
			Matrix4x4 ibp;
			ibp.m[0][0] = ai.a1; ibp.m[0][1] = -ai.b1; ibp.m[0][2] = -ai.c1; ibp.m[0][3] = -ai.d1;
			ibp.m[1][0] = -ai.a2; ibp.m[1][1] = ai.b2; ibp.m[1][2] = ai.c2; ibp.m[1][3] = ai.d2;
			ibp.m[2][0] = -ai.a3; ibp.m[2][1] = ai.b3; ibp.m[2][2] = ai.c3; ibp.m[2][3] = ai.d3;
			ibp.m[3][0] = -ai.a4; ibp.m[3][1] = ai.b4; ibp.m[3][2] = ai.c4; ibp.m[3][3] = ai.d4;
			jointWeightData.inverseBindPoseMatrix = ibp;

			for(uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
				jointWeightData.vertexWeights.push_back(
					{ bone->mWeights[weightIndex].mWeight, bone->mWeights[weightIndex].mVertexId }
				);
			}
		}	
	}

	modelData.rootNode = ReadNode(scene->mRootNode);

	return modelData;
}

MyEngine::Rendering::Node ModelManager::ReadNode(aiNode* node) {
	MyEngine::Rendering::Node result;
	aiVector3D scale;
	aiQuaternion rotate;
	aiVector3D translate;
	node->mTransformation.Decompose(scale, rotate, translate);
	result.transform.scale = { scale.x, scale.y, scale.z };
	result.transform.rotate = { rotate.x, -rotate.y, -rotate.z, rotate.w };
	result.transform.translate = { -translate.x, translate.y, translate.z };
	result.localMatrix = Math::MakeAffineMatrix(
		result.transform.scale, result.transform.rotate, result.transform.translate
	);

	result.name = node->mName.C_Str();

	for(uint32_t i = 0; i < node->mNumMeshes; ++i) {
		result.meshIndices.push_back(node->mMeshes[i]);
	}

	result.children.resize(node->mNumChildren);
	for(uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
		result.children[childIndex] = ReadNode(node->mChildren[childIndex]);
	}

	return result;
}

Animation ModelManager::LoadAnimation(const std::string& directoryPath, const std::string& fileName) {
	Animation animation = {};
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + fileName;
	const aiScene* scene = importer.ReadFile(filePath.c_str(), 0);
	assert(scene->mNumAnimations != 0);
	aiAnimation* animationAssimp = scene->mAnimations[0];
	animation.duration = float(animationAssimp->mDuration / animationAssimp->mTicksPerSecond);

	for(uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex) {
		aiNodeAnim* nodeAnimationAssimp = animationAssimp->mChannels[channelIndex];
		NodeAnimation& nodeAnimation = animation.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];
		for(uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumPositionKeys; ++keyIndex) {
			aiVectorKey& keyAssimp = nodeAnimationAssimp->mPositionKeys[keyIndex];
			KeyframeVector3 keyframe = {};
			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
			keyframe.value = { -keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z };
			nodeAnimation.translate.keyframes.push_back(keyframe);
		}
		for(uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumRotationKeys; ++keyIndex) {
			aiQuatKey& keyAssimp = nodeAnimationAssimp->mRotationKeys[keyIndex];
			KeyframeQuaternion keyframe = {};
			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
			keyframe.value = { keyAssimp.mValue.x, -keyAssimp.mValue.y, -keyAssimp.mValue.z, keyAssimp.mValue.w };
			nodeAnimation.rotate.keyframes.push_back(keyframe);
		}
		for(uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumScalingKeys; ++keyIndex) {
			aiVectorKey& keyAssimp = nodeAnimationAssimp->mScalingKeys[keyIndex];
			KeyframeVector3 keyframe = {};
			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
			keyframe.value = { keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z };
			nodeAnimation.scale.keyframes.push_back(keyframe);
		}
	}
	return animation;
}

Microsoft::WRL::ComPtr<ID3D12Resource> ModelManager::CreateBufferResource(size_t sizeInBytes) {
	D3D12_HEAP_PROPERTIES uploadHeapProperties = {};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeInBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	HRESULT hr = device_->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&resource)
	);
	assert(SUCCEEDED(hr));
	return resource;
}

std::string ModelManager::MakeKey(const std::string& filePath, bool inversion) {
	return filePath + (inversion ? "_inv_true" : "_inv_false");
}
