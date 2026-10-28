#pragma once

struct TextureData {
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;
	DirectX::TexMetadata metadata;
	uint32_t textureIndex = 0; // バインドレスヒープ内の配列番号
	uint32_t refCount = 0;     // 参照カウンタ
	bool isFallback = false;   // 読み込み失敗で別のテクスチャを借りているエントリ (解放しない)
};