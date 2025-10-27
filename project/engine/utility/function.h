#pragma once
#include "../../header/ComPtr.h"
#include "../../header/DX12/dxcapi.h"
#include "../../header/WinSupport.h"
#include "../../header/DX12/directX12.h"
#include "../../header/file.h"
#include "../../externals.h"
#include "struct.h"
#include <memory>

//ログを出力する関数
void Log(std::ofstream& os, const std::string& message);

//DirectX12が返してくる文字列を必要に応じて型変換してあげる
//string->wstring
std::wstring ConvertString(const std::string& str);
//wstring->string
std::string ConvertString(const std::wstring& str);

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

//DescriptorHeapの作成関数
ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

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

//DescriptorHandleを取得する関数(CPUとGPU)
D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);
D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);

//マテリアルファイルの読み込み関数
MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

//ファイル読み込みの関数
ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename, bool inversion = false);