#include "PCH.h"
#include "ShaderManager.h"
#include "Logger.h"
#include "ChangeString.h"

void MyEngine::Rendering::ShaderManager::Initialize (
	IDxcUtils* utils, 
	IDxcCompiler3* compiler,
	IDxcIncludeHandler* includeHandler
) {
	assert(compiler != nullptr && utils != nullptr && includeHandler != nullptr);
	dxcCompiler_ = compiler;
	dxcUtils_ = utils;
	includeHandler_ = includeHandler;
}

uint32_t MyEngine::Rendering::ShaderManager::CompileAndCacheShader (const std::wstring& filePath, const wchar_t* profile) {
	//ファイルパスとプロファイルを組み合わせたキーを生成
	std::wstring key = filePath + L"_" + profile;

	//キーと結びついたものがあるかチェック
	if (pathProfileToID_.count (key)) {
		//存在していればそのキーを返す
		return pathProfileToID_.at(key);
	}

	//無かったらシェーダーをコンパイル
	Microsoft::WRL::ComPtr<IDxcBlob> newBlob = CompilerShader(filePath, profile);

	//キーと結びつけるIDを新規生成した後インクリメント
	uint32_t newID = nextID_++;

	//IDと実体データをキャッシュに登録する
	ShaderInfo newInfo;
	newInfo.ShaderBlob = newBlob;
	newInfo.FilePath = filePath;
	newInfo.Profile = profile;

	//IDと実体を結びつける
	shaderCache_[newID] = std::move (newInfo);

	//逆引き用のマップに登録する
	pathProfileToID_[key] = newID;

	//生成したIDを返す
	return newID;
}

D3D12_SHADER_BYTECODE MyEngine::Rendering::ShaderManager::GetShaderBytecode (uint32_t shaderID) const {
	// GetShaderBytecodeの内部処理
	const auto& info = shaderCache_.at (shaderID);

	D3D12_SHADER_BYTECODE bytecode = {};
	// Blobから直接ポインタとサイズを取得
	bytecode.pShaderBytecode = info.ShaderBlob->GetBufferPointer ();
	bytecode.BytecodeLength = info.ShaderBlob->GetBufferSize ();

	return bytecode;
}

Microsoft::WRL::ComPtr<IDxcBlob> MyEngine::Rendering::ShaderManager::CompilerShader (const std::wstring& filePath, const wchar_t* profile) {
	// hlslファイルを読み込む
	// これからシェーダーをコンパイルする旨をログに出力する
	Logger::GetInstance()->Logger::Log (String::ConvertString (std::format (L"シェーダーコンパイル開始, path:{}, profile:{}\n", filePath, profile)));
	// hlslファイルを読む
	Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
	HRESULT hr = dxcUtils_->LoadFile (filePath.c_str (), nullptr, &shaderSource);
	// 読めなかったらあきらめる
	assert (SUCCEEDED (hr));
	// 読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer = {};
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer ();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize ();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;// UTF_8の文字コードであることを通知

	// compileする
	LPCWSTR arguments[] = {
		filePath.c_str (),			// コンパイル対象のhlslファイル名
		L"-E", L"main",				// エントリーポイントの指定。基本的にmain以外にはしない
		L"-T", profile,				// ShaderProfileの設定
		L"-Zi", L"-Qembed_debug",	// デバッグ用の情報を埋め込む
		L"-Od",						// 最適化を外しておく
		L"-Zpr",					// メモリレイアウトは最優先
	};
	// 実際にshaderをコンパイルする
	Microsoft::WRL::ComPtr<IDxcResult> shaderResult = nullptr;
	hr = dxcCompiler_->Compile (
		&shaderSourceBuffer,		// 読み込んだファイル
		arguments,					// コンパイルオプション
		_countof (arguments),		// コンパイルオプションの数
		includeHandler_,				// includeが含まれた諸々
		IID_PPV_ARGS (shaderResult.GetAddressOf ())	// コンパイル結果
	);
	// コンパイルエラーではなくdxcが起動できないなど致命的な状況
	assert (SUCCEEDED (hr));

	// 警告・エラーが出てたらログに出す
	Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
	shaderResult->GetOutput (DXC_OUT_ERRORS, IID_PPV_ARGS (shaderError.GetAddressOf ()), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength () != 0) {
		Logger::GetInstance()->Logger::Log (shaderError->GetStringPointer ());
	}

	// 実際にコンパイル結果（バイナリ）が取得できたかで判定
	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(shaderBlob.GetAddressOf()), nullptr);

	// バイナリの取得に失敗したときだけ止める
	if (FAILED(hr) || !shaderBlob) {
		assert(false && "シェーダーコンパイルに失敗しました");
		return nullptr;
	}

	return shaderBlob.Get();
}