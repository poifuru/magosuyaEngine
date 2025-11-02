#pragma once
#include <Windows.h>	
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;
#include <dxcapi.h>
#include <strsafe.h>
#include <DbgHelp.h>
#include <memory>
#include "../externals.h"
#include "struct.h"

//クラッシュハンドルを登録するための関数
LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception);

//CompileShader関数
ComPtr<IDxcBlob> CompilerShader(
	//CompilerするShaderファイルへのパス
	const std::wstring& filePath,
	//Compilerに使用するProfile
	const wchar_t* profile,
	//初期化で生成したものを3つ
	const ComPtr<IDxcUtils>& dxcUtils,
	const ComPtr<IDxcCompiler3>& dxcCompiler,
	const ComPtr<IDxcIncludeHandler>& includeHandler,
	//ログを出すための引数
	std::ofstream& os);

//Resource作成関数
ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);



//Textureデータを読みこむ関数
DirectX::ScratchImage LoadTexture(const std::string& filePath);

//DirectXのTexrureResourceを作る関数
ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);

//TextureResourceにデータを転送する関数
[[nodiscard]]
ComPtr<ID3D12Resource> UploadTextureData(
	const ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages,
	ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

//DepthStencilTexture作成関数
ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height);

//ログをテキストで出す関数
std::ofstream Logtext();



//マテリアルファイルの読み込み関数
MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

//ファイル読み込みの関数
ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename, bool inversion = false);