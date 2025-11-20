#pragma once
#include <Windows.h>
#include <wrl.h>
using namespace Microsoft::WRL;
#include <unordered_map>
#include <string>
#include <queue>
#include <vector>
#include "DxCommon.h"
#include "struct.h"

class TextureManager {
public:		//外部公開メソッド
	static TextureManager* GetInstance () {
		////初めて呼び出されたときに一回だけ初期化
		static TextureManager instance;
		return &instance;
	}

	void Initialize (DxCommon* dxCommon);

	//画像をロードする関数
	TextureData* LoadTexture (const std::string& filePath, const std::string& ID);

	//登録した画像のGPUハンドルを取得する関数
	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle (const std::string& ID);

	//テクスチャのアンロード関数
	void UnloadTexture (const std::string& filePath);

	//中間リソース解放関数
	void ClearIntermediateResource ();

	//アクセッサ
	const std::vector<ComPtr<ID3D12Resource>>& GetIntermediateResource () const { return intermediateResource_; }
	const DirectX::TexMetadata& GetMetaData (const std::string& id);

private:
	//コンストラクタを禁止
	TextureManager () = default;
	// コピーコンストラクタと代入演算子を禁止
	TextureManager (const TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;
	TextureManager (TextureManager&&) = delete;
	TextureManager& operator=(TextureManager&&) = delete;

private:	//内部関数
	//LoadTextureのヘルパー関数
	//DirectXのTexrureResourceを作る関数
	ComPtr<ID3D12Resource> CreateTextureResource (const DirectX::TexMetadata& metadata);
	//TextureResourceにデータを転送する関数
	[[nodiscard]]
	ComPtr<ID3D12Resource> UploadTextureData (const ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages);

private:	//メンバ変数
	std::unordered_map<std::string, TextureData> textureMap_;

	//ディスクリプタヒープインデックスの管理
	static UINT nextDescriptorIndex_;
	std::queue<int> freeIndexQueue_;

	//中間リソースの解放待ちリスト
	std::vector<ComPtr<ID3D12Resource>> intermediateResource_;

	//ポインタ借りてくる
	DxCommon* dxCommon_;
};

