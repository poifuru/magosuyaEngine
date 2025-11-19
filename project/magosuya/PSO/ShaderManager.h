#pragma once
#include <Windows.h>
#include <Wrl.h>
using namespace Microsoft::WRL;
#include <d3d12.h>
#include <dxcapi.h>
#include <string>
#include <unordered_map>

class DxCommon;

//シェーダー情報を保持する構造体
struct ShaderInfo {
	ComPtr<IDxcBlob> ShaderBlob;
	std::wstring FilePath;	//シェーダーのファイルパス
	std::wstring Profile;	//コンパイルにつかったプロファイル
};

class ShaderManager {
public:		// メンバ関数
	ShaderManager (DxCommon* dxCommon);
	~ShaderManager ();

	// ファイルパスとプロファイルを受け取ってコンパイルしてIDを返す
	uint32_t CompileAndCasheShader (const std::wstring& filePath, const wchar_t* profile, std::ofstream& os);

	// IDに基づいてD3D12_SHADER_BYTECODEを返す
	D3D12_SHADER_BYTECODE GetShaderBytecode (uint32_t shaderID) const;

private:	// ヘルパー関数
	/// <summary>
	/// シェーダーをコンパイルする関数
	/// </summary>
	/// <param name="filePath">shaderファイルへのパス</param>
	/// <param name="profile">使用するプロファイル</param>
	/// <param name="os">ログストリーム</param>
	/// <returns>shader</returns>
	ComPtr<IDxcBlob> CompilerShader (const std::wstring& filePath, const wchar_t* profile);

private:	// メンバ変数
	//ファイルパスとプロファイルのマップ(ShaderInfoの逆引き用)
	std::unordered_map<std::wstring, uint32_t> m_PathProfileToID;

	//逆引きマップに入れるID(0から連番)
	uint32_t m_NextID = 0;

	//IDとShaderInfoの実体データのマップ
	std::unordered_map<uint32_t, ShaderInfo> m_ShaderCashe;

	//ポインタを借りる
	DxCommon* dxCommon_ = nullptr;
};