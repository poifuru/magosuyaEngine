#include "ShaderManager.h"
#include <cassert>
#include <format>
#include "Logger.h"
#include "ChangeString.h"
#include "engine/DxCommon.h"

ShaderManager::ShaderManager (DxCommon* dxCommon) {
	dxCommon_ = dxCommon;
}

ShaderManager::~ShaderManager () {

}

uint32_t ShaderManager::CompileAndCasheShader (const std::wstring& filePath, const wchar_t* profile) {
	//ファイルパスとプロファイルを組み合わせたキーを生成
	std::wstring key = filePath + L"_" + profile;

	//キーと結びついたものがあるかチェック
	if (m_PathProfileToID.count (key)) {
		//存在していればそのキーを返す
		return m_PathProfileToID.at(key);
	}

	//無かったらシェーダーをコンパイル
	ComPtr<IDxcBlob> newBlob = CompilerShader (filePath, profile);

	//キーと結びつけるIDを新規生成
	uint32_t newID = m_NextID;
	//連番にするのでインクリメント
	m_NextID++;

	//IDと実体データをキャッシュに登録する
	ShaderInfo newInfo;
	newInfo.ShaderBlob = newBlob;
	newInfo.FilePath = filePath;
	newInfo.Profile = profile;

	//逆引き用のマップに登録する
	m_PathProfileToID[key] = newID;

	//生成したIDを返す
	return newID;
}

D3D12_SHADER_BYTECODE ShaderManager::GetShaderBytecode (uint32_t shaderID) const {
	// GetShaderBytecodeの内部処理
	const auto& info = m_ShaderCashe.at (shaderID);

	D3D12_SHADER_BYTECODE bytecode = {};
	// Blobから直接ポインタとサイズを取得
	bytecode.pShaderBytecode = info.ShaderBlob->GetBufferPointer ();
	bytecode.BytecodeLength = info.ShaderBlob->GetBufferSize ();

	return bytecode;
}

ComPtr<IDxcBlob> ShaderManager::CompilerShader (const std::wstring& filePath, const wchar_t* profile) {
	std::ofstream& os = &Logger::Logtext ();

	/*1.hlslファイルを読み込む*/
	//これからシェーダーをコンパイルする旨をログに出力する
	Logger::Log (os, String::ConvertString (std::format (L"Begin CompileShader, path:{}, profile:{}\n", filePath, profile)));
	//hlslファイルを読む
	ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
	HRESULT hr = dxCommon_->GetDxcUtils()->LoadFile (filePath.c_str (), nullptr, &shaderSource);
	//読めなかったらあきらめる
	assert (SUCCEEDED (hr));
	//読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer ();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize ();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;//UTF_8の文字コードであることを通知

	/*2.compileする*/
	LPCWSTR arguments[] = {
		filePath.c_str (),			//コンパイル対象のhlslファイル名
		L"-E", L"main",				//エントリーポイントの指定。基本的にmain以外にはしない
		L"-T", profile,				//ShaderProfileの設定
		L"-Zi", L"-Qembed_debug",	//デバッグ用の情報を埋め込む
		L"-Od",						//最適化を外しておく
		L"-Zpr",					//メモリレイアウトは最優先
	};
	//実際にshaderをコンパイルする
	ComPtr<IDxcResult> shaderResult = nullptr;
	hr = dxCommon_->GetDxcCompiler()->Compile (
		&shaderSourceBuffer,		//読み込んだファイル
		arguments,					//コンパイルオプション
		_countof (arguments),		//コンパイルオプションの数
		dxCommon_->GetIncludeHandler(),				//includeが含まれた諸々
		IID_PPV_ARGS (shaderResult.GetAddressOf ())	//コンパイル結果
	);
	//コンパイルエラーではなくdxcが起動できないなど致命的な状況
	assert (SUCCEEDED (hr));

	/*3.警告・エラーが出ていないか確認する*/
	//警告・エラーが出てたらログに出して止める
	ComPtr<IDxcBlobUtf8> shaderError = nullptr;
	shaderResult->GetOutput (DXC_OUT_ERRORS, IID_PPV_ARGS (shaderError.GetAddressOf ()), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength () != 0) {
		Logger::Log (os, shaderError->GetStringPointer ());
		//警告・エラーダメゼッタイ
		assert (false);
	}

	/*4.compile結果を受け取って返す*/
	//コンパイル結果から実行用のバイナリ部分を取得
	ComPtr<IDxcBlob> shaderBlob = nullptr;
	hr = shaderResult->GetOutput (DXC_OUT_OBJECT, IID_PPV_ARGS (shaderBlob.GetAddressOf ()), nullptr);
	assert (SUCCEEDED (hr));
	//成功したらログを出す
	Logger::Log (os, String::ConvertString (std::format (L"Compile Succeeded, path:{}, profile:{}\n", filePath, profile)));
	//もう使わないリソースを解放
	shaderSource->Release ();
	shaderResult->Release ();
	//実行用のバイナリを返却
	return shaderBlob.Get ();
}