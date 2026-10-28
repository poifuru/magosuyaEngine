#pragma once
#include "TextureData.h"

namespace MyEngine::LowLevel {
	class SrvDescriptorHeapPool;
}

class TextureManager {
public:		//外部公開メソッド
	TextureManager() = default;
	~TextureManager(); // 終了時に残ったテクスチャがあれば解放する

	void Initialize (ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager);

	//画像をロードする関数
	uint32_t LoadTexture (const std::string& filePath, bool isSRGB = true);

	// テクスチャアンロード（使い終わったら参照カウントを減らす）
	void UnloadTexture(const std::string& filePath, bool isSRGB = true);

	//中間リソース解放関数
	void ClearIntermediateResource ();

	// テクスチャID（ファイルパス）からバインドレスヒープ内のインデックスを取得する
	uint32_t GetTextureIndex(const std::string& filePath, bool isSRGB = true) const;

	/// <summary>
	/// メモリ上の画素からテクスチャを作る（フォントアトラスなど、ファイルが無い画像用）
	/// sRGBなし(Linear)として登録するので、GetTextureIndex(name, false) で引ける
	/// </summary>
	/// <param name="name">登録名（ファイルパスと被らない名前にする）</param>
	/// <param name="width">幅 px</param>
	/// <param name="height">高さ px</param>
	/// <param name="format">画素のフォーマット（フォントなら DXGI_FORMAT_R8_UNORM）</param>
	/// <param name="pixels">画素データの先頭</param>
	/// <param name="rowPitch">1行あたりのバイト数</param>
	/// <returns>バインドレスヒープ内のインデックス</returns>
	uint32_t CreateTextureFromMemory(const std::string& name, uint32_t width, uint32_t height, DXGI_FORMAT format, const void* pixels, size_t rowPitch);

public:
	// コピー・移動禁止
	TextureManager(const TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;
	TextureManager(TextureManager&&) = delete;
	TextureManager& operator=(TextureManager&&) = delete;

private:	//内部関数
	// 内部用ヘルパー関数群
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);

	Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const Microsoft::WRL::ComPtr<ID3D12Resource>& texture,
		const DirectX::ScratchImage& mipImages
	);

	// マップキー作成関数
	std::string MakeKey(const std::string& filePath, bool isSRGB);

private:	//メンバ変数
	// テクスチャID（ファイルパス）と実体データのマップ
	std::unordered_map<std::string, TextureData> textureMap_;

	// GPUにデータをコピーする間だけ生きている必要がある一時的なバッファのゴミ箱
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> intermediateResources_;

	ID3D12Device* device_ = nullptr;
	ID3D12GraphicsCommandList* cmdList_ = nullptr;
	MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager_ = nullptr;
};

