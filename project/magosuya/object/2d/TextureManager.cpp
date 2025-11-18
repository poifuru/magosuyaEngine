#include "TextureManager.h"
#include <d3dx12.h>
#include <DirectXTex.h>
#include <filesystem>
#include "ChangeString.h"

UINT TextureManager::nextDescriptorIndex_ = 1;

TextureManager::TextureManager (DxCommon* dxCommon) {
	dxCommon_ = dxCommon;
}

TextureManager::~TextureManager () {

}

TextureData* TextureManager::LoadTexture (const std::string& filePath, const std::string& ID) {
	//そのパスの画像をすでに読み込んでいたら
	if (textureMap_.count (filePath)) {
		//既存データを取得
		TextureData& existingData = textureMap_.at (filePath);
		//参照カウントを増やす
		existingData.ref_count++;
		//存在していたら既存のデータを返す
		return &existingData;
	}

	//returnするデータを詰める箱
	TextureData newData{};
	newData.ref_count = 1;

	//実際に読み込む処理
	//テクスチャファイルを読み込んでプログラムで扱えるようにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = String::ConvertString (filePath);
	OutputDebugStringW ((L"探してるファイル: " + filePathW + L"\n").c_str ());
	HRESULT hr = DirectX::LoadFromWICFile (filePathW.c_str (), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	if (FAILED (hr)) {
		std::wstringstream ss;
		ss << L"[エラー] テクスチャ読み込み失敗！HRESULT: 0x" << std::hex << hr << std::endl;
		OutputDebugStringW (ss.str ().c_str ());
	}
	assert (SUCCEEDED (hr));

	//ミップマップの作成
	DirectX::ScratchImage mipImage{};
	hr = DirectX::GenerateMipMaps (image.GetImages (), image.GetImageCount (), image.GetMetadata (), DirectX::TEX_FILTER_SRGB, 0, mipImage);
	assert (SUCCEEDED (hr));

	//mipImageを使ってmetaDataを作る
	newData.metadata = mipImage.GetMetadata ();
	//作ったmetaDataをもとにテクスチャリソースを作成
	newData.textureResource = CreateTextureResource (newData.metadata);
	//実際にデータを転送
	intermediateResource_.push_back(UploadTextureData (newData.textureResource, mipImage));

	//metaDataをもとにSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = newData.metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	srvDesc.Texture2D.MipLevels = UINT (newData.metadata.mipLevels);

	//SRVを作成するDescriptorHeapの場所を決める
	UINT newIndex;

	//キューの空きリストをチェック
	if (!freeIndexQueue_.empty ()) {
		//空きがあればそこを使う
		newIndex = freeIndexQueue_.front ();
		freeIndexQueue_.pop ();	//キューから取り除く
	}
	else {
		//空きがなかったら、次に割り当てるインデックスを使う
		newIndex = nextDescriptorIndex_;
		nextDescriptorIndex_++;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU;
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU;
	textureSrvHandleCPU = dxCommon_->GetSRVCPUDescriptorHandle (newIndex);
	textureSrvHandleGPU = dxCommon_->GetSRVGPUDescriptorHandle (newIndex);

	//実際にSRVを生成
	dxCommon_->GetDevice ()->CreateShaderResourceView (newData.textureResource.Get (), &srvDesc, textureSrvHandleCPU);

	//生成物をmapに渡すためにデータを詰める
	newData.handle = textureSrvHandleGPU;
	//どのインデックスを使ったかを保存しておくと解放時に便利
	newData.descriptorIndex = newIndex;

	//mapに登録
	textureMap_[ID] = newData;
	return &textureMap_.at (ID);
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetTextureHandle (const std::string& ID) {
	//ID指定してmapから持ってくる
	assert (textureMap_.count (ID));
	return textureMap_.at (ID).handle;
}

void TextureManager::UnloadTexture (const std::string& filePath) {
	//参照カウントを減らす
	if (!textureMap_.count (filePath)) { return; }
	textureMap_.at (filePath).ref_count--;

	// 参照を取得する
	TextureData& data = textureMap_.at (filePath);

	//参照カウントがゼロになったらテクスチャ削除
	if (data.ref_count <= 0) {
		//GPUリソースはComPtrで自動開放
		//使っていたインデックスを空きリストに戻す
		freeIndexQueue_.push (data.descriptorIndex);
		//キャッシュマップからデータを削除
		textureMap_.erase (filePath);
	}
}

void TextureManager::ClearIntermediateResource () {
	intermediateResource_.clear ();
}

const DirectX::TexMetadata& TextureManager::GetMetaData (const std::string& id) {
	assert (textureMap_.count (id));

	return textureMap_.at (id).metadata;
}

ComPtr<ID3D12Resource> TextureManager::CreateTextureResource (const DirectX::TexMetadata& metadata) {
	//1.metadataを基にResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT (metadata.width);								//Textureの幅
	resourceDesc.Height = UINT (metadata.height);							//Textureの高さ
	resourceDesc.MipLevels = UINT (metadata.mipLevels);						//mipmapの数
	resourceDesc.DepthOrArraySize = UINT (metadata.arraySize);				//奥行　or　配列Textureの配列数
	resourceDesc.Format = metadata.format;									//TextureのFormat
	resourceDesc.SampleDesc.Count = 1;										//サンプリングカウント。1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION (metadata.dimension);	//Textureの次元数。普段使っているのは2次元

	//2.利用するHeapの設定。非常に特殊な運用。
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;			//細かい設定を行う

	//3.Resourceを生成する
	ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = dxCommon_->GetDevice ()->CreateCommittedResource (
		&heapProperties,					//Heapの設定
		D3D12_HEAP_FLAG_NONE,				//Heapの特殊な設定。特になし。
		&resourceDesc,						//Resourceの設定
		D3D12_RESOURCE_STATE_COPY_DEST,		//データ転送される設定
		nullptr,							//Clear最適値。使わないのでnullptr。
		IID_PPV_ARGS (resource.GetAddressOf ()));			//作成するResourceポインタへのポインタ
	assert (SUCCEEDED (hr));
	return resource;
}

ComPtr<ID3D12Resource> TextureManager::UploadTextureData (const ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages) {
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload (dxCommon_->GetDevice (), mipImages.GetImages (), mipImages.GetImageCount (), mipImages.GetMetadata (), subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize (texture.Get (), 0, UINT (subresources.size ()));
	ComPtr<ID3D12Resource> intermediateResource = dxCommon_->CreateBufferResource (intermediateSize);
	UpdateSubresources (dxCommon_->GetCommandList (), texture.Get (), intermediateResource.Get (), 0, 0, UINT (subresources.size ()), subresources.data ());

	//Textureへの転送後は利用できるよう、D3D12_RESOURCE_STATE_COPY_DESTからD3D12_RESOURCE_STATE_GENERIC_READへResourceStateを変更する
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture.Get ();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	dxCommon_->GetCommandList ()->ResourceBarrier (1, &barrier);
	return intermediateResource;
}