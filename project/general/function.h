#pragma once
#include <Windows.h>	
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;
#include <dxcapi.h>
#include <strsafe.h>
#include <DbgHelp.h>
#include <memory>
#include "struct.h"

//クラッシュハンドルを登録するための関数
LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception);

//DepthStencilTexture作成関数
ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height);

//ログをテキストで出す関数
std::ofstream Logtext();

//マテリアルファイルの読み込み関数
MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

//ファイル読み込みの関数
ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename, bool inversion = false);