#include "DxCommon.h"
#include <dxcapi.h>
#include <d3d12sdklayers.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxcompiler.lib")

#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#include <filesystem>
#include <cassert>
#include <thread>
#include "function.h"
#include "Logger.h"
#include "ChangeString.h"

DxCommon::DxCommon () {

}

DxCommon::~DxCommon () {

}

void DxCommon::Initialize () {
	HRESULT hr;
	std::ofstream logStream = Logger::Logtext ();

	// 1.システムレベルの初期化
	//誰も補足しなかった場合に(Unhandled)、補足する関数を登録
	//main関数が始まってすぐに登録すると良い
	SetUnhandledExceptionFilter (ExportDump);

	//COMの初期化
	hr = CoInitializeEx (0, COINIT_MULTITHREADED);

	winApi_ = std::make_unique<WindowsAPI> ();
	winApi_->Initialize ();

	// 2.FPSの固定化
	InitializeFixFPS ();

	// 3.基本オブジェクトの初期化
	CreateDevice ();
	CreateCommand ();
	CreateDxcCompiler ();
	CreateFence ();

	// 4.リソースとヒープの初期化
	CreateDescriptorHeap ();
	CreateSwapChain ();
	CreateDepthBaffer ();

	// 5.ビューと設定の初期化
	CreateRTV ();
	CreateDSV ();
	ViewportRectInit ();
	ScissorRectInit ();

	
}

void DxCommon::BeginFrame () {
	//これから書きこむバックバッファのインデックスを取得
	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex ();
	//TransitionBarrierの設定
	D3D12_RESOURCE_BARRIER barrier{};
	//今回のバリアはTransition
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	//Noneにしておく
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	//バリアを張る対象のリソース。現在のバックバッファに対して行う
	barrier.Transition.pResource = swapChainResources[backBufferIndex].Get ();
	//遷移前(現在)のResourceState
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	//遷移後のResourceState
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	//TransitionBarrierを張る
	commandList->ResourceBarrier (1, &barrier);
	//描画先のRTVを指定する
	commandList->OMSetRenderTargets (1, &rtvHandles[backBufferIndex], false, nullptr);
	//描画先のRTVとDSVを設定する
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart ();
	commandList->OMSetRenderTargets (1, &rtvHandles[backBufferIndex], false, &dsvHandle);
	//指定した色で画面全体をクリアする
	float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };	//青っぽい色。RGBAの順
	commandList->ClearRenderTargetView (rtvHandles[backBufferIndex], clearColor, 0, nullptr);
	//指定した深度で画面全体をクリアする
	commandList->ClearDepthStencilView (dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	//ImGui描画用のDescriptorHeapの設定
	ComPtr<ID3D12DescriptorHeap> descriptorHeaps[] = { srvDescriptorHeap.Get () };
	commandList->SetDescriptorHeaps (1, descriptorHeaps->GetAddressOf ());
	//Viewportを設定
	commandList->RSSetViewports (1, &viewport);
	//Scissorを設定
	commandList->RSSetScissorRects (1, &scissorRect);
}

void DxCommon::EndFrame () {
	//これから書きこむバックバッファのインデックスを取得
	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex ();
	//画面に描く処理はすべて終わり、画面に映すので、状態を遷移
	D3D12_RESOURCE_BARRIER barrier{};
	//今回はRenderTargetからPresentにする
	//バリアを張る対象のリソース。現在のバックバッファに対して行う
	barrier.Transition.pResource = swapChainResources[backBufferIndex].Get ();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	//TransitionBarrierを張る
	commandList->ResourceBarrier (1, &barrier);
	//コマンドリストの内容を確定させる。すべてのコマンドを詰んでからCloseすること
	HRESULT hr = commandList->Close ();
	assert (SUCCEEDED (hr));

	//GPUにコマンドリストの実行を行わせる
	ComPtr<ID3D12CommandList> commandLists[] = { commandList.Get () };
	commandQueue->ExecuteCommandLists (1, commandLists->GetAddressOf ());
	//GPUとOSに画面の交換を行うように通知する
	swapChain->Present (1, 0);
	//Fenceの値を更新する
	fenceValue++;
	//GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようにSignalを送る
	commandQueue->Signal (fence.Get (), fenceValue);
	//Fenceの値が指定したSignal値にたどり着いているか確認する
	//GetCompletedValueの初期値はFence作成時に渡した初期値
	if (fence->GetCompletedValue () < fenceValue) {
		//指定した値にたどり着いていないので、たどり着くまで待つようにイベントを設定する
		fence->SetEventOnCompletion (fenceValue, fenceEvent);
		//イベント待つ
		WaitForSingleObject (fenceEvent, INFINITE);
	}
	//FPS固定:更新処理
	UpdateFixFPS ();
	//次のフレーム用のコマンドリストを準備
	hr = commandAllocator->Reset ();
	assert (SUCCEEDED (hr));
	hr = commandList->Reset (commandAllocator.Get (), nullptr);
	assert (SUCCEEDED (hr));
}

void DxCommon::Finalize () {
	CloseHandle (fenceEvent);

	winApi_->Finalize ();
	CoUninitialize ();
}

DirectX::ScratchImage DxCommon::LoadTexture (const std::string& filePath) {
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
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps (image.GetImages (), image.GetImageCount (), image.GetMetadata (), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert (SUCCEEDED (hr));

	//ミップマップ付きのデータを返す
	return mipImages;
}

ComPtr<IDxcBlob> DxCommon::CompilerShader (const std::wstring& filePath, const wchar_t* profile, std::ofstream& os) {
	/*1.hlslファイルを読み込む*/
	//これからシェーダーをコンパイルする旨をログに出力する
	Logger::Log (os, String::ConvertString (std::format (L"Begin CompileShader, path:{}, profile:{}\n", filePath, profile)));
	//hlslファイルを読む
	ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile (filePath.c_str (), nullptr, &shaderSource);
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
	hr = dxcCompiler->Compile (
		&shaderSourceBuffer,		//読み込んだファイル
		arguments,					//コンパイルオプション
		_countof (arguments),		//コンパイルオプションの数
		includeHandler.Get(),				//includeが含まれた諸々
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

ComPtr<ID3D12Resource> DxCommon::CreateBufferResource (size_t sizeInBytes) {
	assert (device != nullptr);

	// アップロードヒープのプロパティ
	D3D12_HEAP_PROPERTIES uploadHeapProperties = {};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	uploadHeapProperties.CreationNodeMask = 1;
	uploadHeapProperties.VisibleNodeMask = 1;

	// リソースのディスクリプション（バッファとして）
	D3D12_RESOURCE_DESC vertexResourceDesc = {};
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Alignment = 0;
	vertexResourceDesc.Width = sizeInBytes;
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	vertexResourceDesc.SampleDesc.Count = 1;
	vertexResourceDesc.SampleDesc.Quality = 0;
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	vertexResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// リソースの作成
	ComPtr<ID3D12Resource> vertexResource = nullptr;
	HRESULT hr = device->CreateCommittedResource (
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&vertexResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS (vertexResource.GetAddressOf ())
	);
	assert (SUCCEEDED (hr));

	return vertexResource;
}

ComPtr<ID3D12DescriptorHeap> DxCommon::CreateDescriptorHeap (D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {
	ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap (&descriptorHeapDesc, IID_PPV_ARGS (descriptorHeap.GetAddressOf ()));
	assert (SUCCEEDED (hr));
	return descriptorHeap;
}

D3D12_CPU_DESCRIPTOR_HANDLE DxCommon::GetSRVCPUDescriptorHandle (uint32_t index) {
	return GetCPUDescriptorHandle (srvDescriptorHeap, rtvDescriptorHeapSize_, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE DxCommon::GetSRVGPUDescriptorHandle (uint32_t index) {
	return GetGPUDescriptorHandle (srvDescriptorHeap, rtvDescriptorHeapSize_, index);
}

void DxCommon::InitializeFixFPS () {
	reference_ = std::chrono::steady_clock::now ();
}

void DxCommon::CreateDevice () {
	HRESULT hr;
	std::ofstream logStream = Logger::Logtext ();

	//デバイス生成の前にデバッグレイヤーを有効化する
#if defined (Debug) || (Development)
	ComPtr<ID3D12Debug1> debugController = nullptr;
	if (SUCCEEDED (D3D12GetDebugInterface (IID_PPV_ARGS (debugController.GetAddressOf ())))) {
		//デバッグレイヤーを有効化する
		debugController->EnableDebugLayer ();
		//さらにGPU側でもチェックを行うようにする
		debugController->SetEnableGPUBasedValidation (true);
	}
#endif //DEBUG

	//dxgiFactory生成
	//HRESULTはWindows系のエラーコードであり、
	//関数が成功したかどうかをSUCCEDEDマクロで判定できる
	hr = CreateDXGIFactory (IID_PPV_ARGS (dxgiFactory.GetAddressOf ()));
	//初期化の根本的な部分でエラーが出た場合はプログラムが間違っているか、
	//どうにもできない場合が多いのでassertにしておく
	assert (SUCCEEDED (hr));

	//アダプタ生成
	//良い順にアダプタを頼む
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference (i,
		 DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS (useAdapter.GetAddressOf ())) !=
		 DXGI_ERROR_NOT_FOUND; ++i) {
		//アダプターの情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3 (&adapterDesc);
		assert (SUCCEEDED (hr)); //取得できないのは一大事
		//ソフトウェアアダプタでなければ採用！
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
			//採用したアダプタの情報をログに出力。wstringの方なので注意
			Logger::Log (logStream, String::ConvertString (std::format (L"Use Adapter:{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr;	//ソフトウェアアダプタの場合は見なかったことにする
	}
	//適切なアダプタが見つからなかったので起動できない
	assert (useAdapter != nullptr);

	//デバイス生成
	//機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0
	};
	const char* featureLevelString[] = { "12.2", "12.1", "12.0" };
	//高い順に生成できるか試していく
	for (size_t i = 0; i < _countof (featureLevels); ++i) {
		hr = D3D12CreateDevice (useAdapter.Get (), featureLevels[i], IID_PPV_ARGS (device.GetAddressOf ()));
		//指定した機能レベルでデバイスが生成できたかを確認
		if (SUCCEEDED (hr)) {
			//生成できたのでログ出力を行ってループを抜ける
			Logger::Log (logStream, std::format ("FeatureLevel : {}\n", featureLevelString[i]));
			break;
		}
	}
	//デバイスの生成がうまくいかなかったので起動できない
	assert (device != nullptr);
	//初期化完了のログを出す
	Logger::Log (logStream, "Complete create D3D12Device!!!\n");

	//警告とかエラーとかが出たら出力ログに出してくれるらしい
#if defined (Debug) || (Development)
	ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
	if (SUCCEEDED (device->QueryInterface (IID_PPV_ARGS (infoQueue.GetAddressOf ())))) {
		//ヤバいエラー時に止まる
		infoQueue->SetBreakOnSeverity (D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		//エラー時に止まる
		infoQueue->SetBreakOnSeverity (D3D12_MESSAGE_SEVERITY_ERROR, true);
		//警告時に止まる
		//infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

		//抑制するメッセージのID
		D3D12_MESSAGE_ID denyIds[] = {
			//Windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用バグによるエラーメッセージ
			//https://stackoverflow.com/questions/69805245/directx-12-application-is-crashing-in-windows-11
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};
		//抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof (denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof (severities);
		filter.DenyList.pSeverityList = severities;
		//指定したメッセージの表示を抑制する
		infoQueue->PushStorageFilter (&filter);
	}
#endif // _DEBUG
}

void DxCommon::CreateCommand () {
	HRESULT hr;

	//コマンドアロケータ生成
	hr = device->CreateCommandAllocator (D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS (commandAllocator.GetAddressOf ()));
	//コマンドアロケータの生成がうまくいかなかったので起動できない
	assert (SUCCEEDED (hr));

	//コマンドリスト生成
	hr = device->CreateCommandList (0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get (), nullptr, IID_PPV_ARGS (commandList.GetAddressOf ()));
	//コマンドリストの生成がうまくいかなかったので起動できない
	assert (SUCCEEDED (hr));

	//コマンドキュー生成
	hr = device->CreateCommandQueue (&commandQueueDesc, IID_PPV_ARGS (commandQueue.GetAddressOf ()));
	//コマンドキューの生成がうまくいかなかったので起動できない
	assert (SUCCEEDED (hr));
}

void DxCommon::CreateDxcCompiler () {
	HRESULT hr;

	//dxcCompilerの生成
	hr = DxcCreateInstance (CLSID_DxcUtils, IID_PPV_ARGS (dxcUtils.GetAddressOf ()));
	assert (SUCCEEDED (hr));
	hr = DxcCreateInstance (CLSID_DxcCompiler, IID_PPV_ARGS (dxcCompiler.GetAddressOf ()));
	assert (SUCCEEDED (hr));

	//後のインクルードに対応する設定
	hr = dxcUtils->CreateDefaultIncludeHandler (includeHandler.GetAddressOf ());
	assert (SUCCEEDED (hr));
}

void DxCommon::CreateFence () {
	HRESULT hr;

	//フェンス生成
	hr = device->CreateFence (fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS (fence.GetAddressOf ()));
	assert (SUCCEEDED (hr));

	//イベント作成
	fenceEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
	assert (fenceEvent != nullptr);
}

void DxCommon::CreateDescriptorHeap () {
	//ディスクリプターヒープサイズを取得する
	rtvDescriptorHeapSize_ = device->GetDescriptorHandleIncrementSize (D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	srvDescriptorHeapSize_ = device->GetDescriptorHandleIncrementSize (D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	dsvDescriptorHeapSize_ = device->GetDescriptorHandleIncrementSize (D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	//RTVディスクリプタヒープ生成
	rtvDescriptorHeap = CreateDescriptorHeap (D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	//SRV用ディスクリプタ生成。ディスクリプタの数は128。SRVはShader内で触るものなので、ShaderVisibleはtrue
	srvDescriptorHeap = CreateDescriptorHeap (D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
	//DSVディスクリプタヒープ生成
	dsvDescriptorHeap = CreateDescriptorHeap (D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
}

void DxCommon::CreateSwapChain () {
	HRESULT hr;

	//スワップチェーンを設定する
	swapChainDesc.Width = winApi_->kClientWidth;		//画面の幅,ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Height = winApi_->kClientHeight;	//画面の高さ,ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	//色の形式
	swapChainDesc.SampleDesc.Count = 1;	//マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	//描画のターゲットとして利用する
	swapChainDesc.BufferCount = 2;	//ダブルバッファ
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	//モニタにうつしたら、中身を破棄
	//コマンドキュー,ウィンドウハンドル,設定を渡して生成する
	hr = dxgiFactory->CreateSwapChainForHwnd (commandQueue.Get (), winApi_->GetHwnd (), &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf ()));
	assert (SUCCEEDED (hr));
}

void DxCommon::CreateDepthBaffer () {
	//DepthStencilTextureをウィンドウサイズで作成
	depthStencilResource = CreateDepthStencilTextureResource (device.Get (), winApi_->kClientWidth, winApi_->kClientHeight);
}

void DxCommon::CreateRTV () {
	HRESULT hr;

	//swapChainからResourceを引っ張ってくる : うまく取得出来なければ起動できない
	hr = swapChain->GetBuffer (0, IID_PPV_ARGS (&swapChainResources[0]));
	assert (SUCCEEDED (hr));
	hr = swapChain->GetBuffer (1, IID_PPV_ARGS (&swapChainResources[1]));
	assert (SUCCEEDED (hr));

	//RTVを設定する
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;	//出力結果をSRGBに変換して書き込む
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;		//2Dテクスチャとして書き込む
	//アドレス全体の幅
	const UINT handleIncrementSize = device->GetDescriptorHandleIncrementSize (D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//ディスクリプタの先頭を取得する(ループで使うために現在のハンドルとする)
	D3D12_CPU_DESCRIPTOR_HANDLE currentHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart ();

	//スワップチェーンリソースの数
	const UINT RTV_COUNT = swapChainDesc.BufferCount;

	for (UINT i = 0; i < RTV_COUNT; i++) {
		//ディスクリプタの先頭アドレスに書き込む
		rtvHandles[i] = currentHandle;
		//生成
		device->CreateRenderTargetView (swapChainResources[i].Get (), &rtvDesc, rtvHandles[i]);
		//次の書き込み場所を教えてあげる
		currentHandle.ptr += handleIncrementSize;
	}
}

void DxCommon::CreateDSV () {
	//DSVの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};

	//DSVの設定
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;		//Format。基本的にはResourceに合わせる
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;		//2dTexture

	//DSVHeapの先頭にDSVを作る
	device->CreateDepthStencilView (depthStencilResource.Get (), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart ());
}

void DxCommon::ViewportRectInit () {
	//ビューポート設定
	//クライアント領域のサイズと一緒にして画面全体に表示
	viewport.Width = static_cast<float>(winApi_->kClientWidth);
	viewport.Height = static_cast<float>(winApi_->kClientHeight);
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
}

void DxCommon::ScissorRectInit () {
	//シザー矩形設定
	//基本的にビューポートと同じ矩形が構成されるようにする
	scissorRect.left = 0;
	scissorRect.right = winApi_->kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = winApi_->kClientHeight;
}

void DxCommon::UpdateFixFPS () {
	//1/60秒ぴったりの時間
	const std::chrono::microseconds kMinTime (uint64_t (1000000.0f / 60.0f));
	//1/60秒よりわずかに短い時間
	const std::chrono::microseconds kMinCheckTime (uint64_t (1000000.0f / 65.0f));

	//実際の更新処理
	//現在時間を取得する
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now ();
	//前回記録からの経過時間を取得する
	std::chrono::microseconds elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - reference_);

	//1/60秒(よりわずかに短い時間) 立っていない場合
	if (elapsed < kMinCheckTime) {
		//1/60秒経過するまで微小なスリープを繰り返す
		while (std::chrono::steady_clock::now () - reference_ < kMinTime) {
			//1マイクロ秒スリープ
			std::this_thread::sleep_for (std::chrono::microseconds (1));
		}
	}
	//現在の時間を記録する
	reference_ = std::chrono::steady_clock::now ();
}

D3D12_CPU_DESCRIPTOR_HANDLE DxCommon::GetCPUDescriptorHandle (const ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_CPU_DESCRIPTOR_HANDLE    handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart ();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE DxCommon::GetGPUDescriptorHandle (const ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_GPU_DESCRIPTOR_HANDLE    handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart ();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}