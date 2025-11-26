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
	static ShaderManager* GetInstance () {
		//初めて呼び出されたときに一回だけ初期化
		static ShaderManager instance;
		return &instance;
	}

	void Initialize (DxCommon* dxCommon);

	// ファイルパスとプロファイルを受け取ってコンパイルしてIDを返す
	uint32_t CompileAndCasheShader (const std::wstring& filePath, const wchar_t* profile);

	// IDに基づいてD3D12_SHADER_BYTECODEを返す
	D3D12_SHADER_BYTECODE GetShaderBytecode (uint32_t shaderID) const;

private:
	//コンストラクタを禁止
	ShaderManager () = default;
	// コピーコンストラクタと代入演算子を禁止
	ShaderManager (const ShaderManager&) = delete;
	ShaderManager& operator=(const ShaderManager&) = delete;
	ShaderManager (ShaderManager&&) = delete;
	ShaderManager& operator=(ShaderManager&&) = delete;

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

	//逆引きマップに入れるID(0は無効なリソースとして扱われてしまう可能性があるので1から連番)
	uint32_t m_NextID = 1;

	//IDとShaderInfoの実体データのマップ
	std::unordered_map<uint32_t, ShaderInfo> m_ShaderCashe;

	//ポインタを借りる
	DxCommon* dxCommon_ = nullptr;
};