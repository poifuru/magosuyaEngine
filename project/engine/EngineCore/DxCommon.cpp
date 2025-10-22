#include "DxCommon.h"
#include "../utility/function.h"
#include <d3d12sdklayers.h>

void DxCommon::Initialize () {
	//ウィンドウサイズを表す構造体にクライアント領域を入れる
	RECT wrc = { 0, 0, kClientWidth, kClientHeight };

	//COMの初期化
	CoInitializeEx (0, COINIT_MULTITHREADED);

	//誰も補足しなかった場合に(Unhandled)、補足する関数を登録
	//main関数が始まってすぐに登録すると良い
	SetUnhandledExceptionFilter (ExportDump);

	std::ofstream logStream = Logtext ();

	//ウィンドウプロシージャ
	wc.lpfnWndProc = WindowProc;
	//ウィンドウクラス名
	wc.lpszClassName = L"LE2B_22_マスヤ_ゴウ";
	//インスタンスハンドル
	wc.hInstance = GetModuleHandle (nullptr);
	//カーソル
	wc.hCursor = LoadCursor (nullptr, IDC_ARROW);

	//ウィンドウクラスを登録する
	RegisterClass (&wc);

	//クライアント領域を元に実際のサイズにwrcを変更してもらう
	AdjustWindowRect (&wrc, WS_OVERLAPPEDWINDOW, false);

	//ウィンドウを生成
	hwnd = CreateWindow (
		wc.lpszClassName,		//利用するクラス名
		L"CG2",					//タイトルバーの文字
		WS_OVERLAPPEDWINDOW,	//よく見るウィンドウスタイル
		CW_USEDEFAULT,			//表示x座標(Windowsに任せる)
		CW_USEDEFAULT,			//表示y座標(WindowsOSに任せる)
		wrc.right - wrc.left,	//ウィンドウ横幅
		wrc.bottom - wrc.top,	//ウィンドウ縦幅
		nullptr,				//親ウィンドウハンドル
		nullptr,				//メニューハンドル
		wc.hInstance,			//インスタンスハンドル
		nullptr					//オプション
	);

	//ウィンドウを表示
	ShowWindow (hwnd, SW_SHOW);

	//dxgiFactory生成
	//HRESULTはWindows系のエラーコードであり、
	//関数が成功したかどうかをSUCCEDEDマクロで判定できる
	HRESULT hr = CreateDXGIFactory (IID_PPV_ARGS (dxgiFactory.GetAddressOf ()));
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
			Log (logStream, ConvertString (std::format (L"Use Adapter:{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr;	//ソフトウェアアダプタの場合は見なかったことにする
	}
	//適切なアダプタが見つからなかったので起動できない
	assert (useAdapter != nullptr);

	//dxcCompilerの生成
	hr = DxcCreateInstance (CLSID_DxcUtils, IID_PPV_ARGS (dxcUtils.GetAddressOf ()));
	assert (SUCCEEDED (hr));
	hr = DxcCreateInstance (CLSID_DxcCompiler, IID_PPV_ARGS (dxcCompiler.GetAddressOf ()));
	assert (SUCCEEDED (hr));

	//後のインクルードに対応する設定
	hr = dxcUtils->CreateDefaultIncludeHandler (includeHandler.GetAddressOf ());
	assert (SUCCEEDED (hr));

	//デバイス生成の前にデバッグレイヤーを有効化する
#ifdef _DEBUG
	ComPtr<ID3D12Debug1> debugController = nullptr;
	if (SUCCEEDED (D3D12GetDebugInterface (IID_PPV_ARGS (debugController.GetAddressOf ())))) {
		//デバッグレイヤーを有効化する
		debugController->EnableDebugLayer ();
		//さらにGPU側でもチェックを行うようにする
		debugController->SetEnableGPUBasedValidation (true);
	}
#endif //DEBUG

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
			Log (logStream, std::format ("FeatureLevel : {}\n", featureLevelString[i]));
			break;
		}
	}
	//デバイスの生成がうまくいかなかったので起動できない
	assert (device != nullptr);
	//初期化完了のログを出す
	Log (logStream, "Complete create D3D12Device!!!\n");

	//警告とかエラーとかが出たら出力ログに出してくれるらしい
#ifdef _DEBUG
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

	//コマンドキュー生成
	hr = device->CreateCommandQueue (&commandQueueDesc, IID_PPV_ARGS (commandQueue.GetAddressOf ()));
	//コマンドキューの生成がうまくいかなかったので起動できない
	assert (SUCCEEDED (hr));

	//コマンドアロケータ生成
	hr = device->CreateCommandAllocator (D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS (commandAllocator.GetAddressOf ()));
	//コマンドアロケータの生成がうまくいかなかったので起動できない
	assert (SUCCEEDED (hr));

	//コマンドリスト生成
	hr = device->CreateCommandList (0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get (), nullptr, IID_PPV_ARGS (commandList.GetAddressOf ()));
	//コマンドリストの生成がうまくいかなかったので起動できない
	assert (SUCCEEDED (hr));

	//スワップチェーンを生成する
	swapChainDesc.Width = kClientWidth;		//画面の幅,ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Height = kClientHeight;	//画面の高さ,ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	//色の形式
	swapChainDesc.SampleDesc.Count = 1;	//マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	//描画のターゲットとして利用する
	swapChainDesc.BufferCount = 2;	//ダブルバッファ
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	//モニタにうつしたら、中身を破棄
	//コマンドキュー,ウィンドウハンドル,設定を渡して生成する
	hr = dxgiFactory->CreateSwapChainForHwnd (commandQueue.Get (), hwnd, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf ()));
	assert (SUCCEEDED (hr));

	//swapChainからResourceを引っ張ってくる
	hr = swapChain->GetBuffer (0, IID_PPV_ARGS (&swapChainResources[0]));
	//うまく取得出来なければ起動できない
	assert (SUCCEEDED (hr));
	hr = swapChain->GetBuffer (1, IID_PPV_ARGS (&swapChainResources[1]));
	assert (SUCCEEDED (hr));

	//RTVディスクリプタヒープ生成
	rtvDescriptorHeap = CreateDescriptorHeap (device.Get (), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	//RTVを設定する
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;	//出力結果をSRGBに変換して書き込む
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;		//2Dテクスチャとして書き込む
	//ディスクリプタの先頭を取得する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart ();

	//まず1つ目を作る。1つ目は最初の所に作る。作る場所をこちらで指定してあげる必要がある
	rtvHandles[0] = rtvStartHandle;
	device->CreateRenderTargetView (swapChainResources[0].Get (), &rtvDesc, rtvHandles[0]);
	//2つ目のディスクリプタハンドルを得る(自力で)
	rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize (D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//2つ目を作る
	device->CreateRenderTargetView (swapChainResources[1].Get (), &rtvDesc, rtvHandles[1]);

	//SRV用ディスクリプタ生成。ディスクリプタの数は128。SRVはShader内で触るものなので、ShaderVisibleはtrue
	srvDescriptorHeap = CreateDescriptorHeap (device.Get (), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

	//フェンス生成
	hr = device->CreateFence (fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS (fence.GetAddressOf ()));
	assert (SUCCEEDED (hr));

	//イベント作成
	fenceEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
	assert (fenceEvent != nullptr);

	// DiscriptorRangeの設定
	descriptorRange[0].BaseShaderRegister = 0;	//0から始まる
	descriptorRange[0].NumDescriptors = 1;		//数は1つ
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;	//SRVを使う
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;	//Offsetを自動計算

	//RootSignature
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	//RootParameter作成。複数設定できるので配列
	rootParameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;		//CBVを使う
	rootParameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;	//VertexShaderで使う
	rootParameter[0].Descriptor.ShaderRegister = 0;

	rootParameter[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;		//CBVを使う
	rootParameter[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	//PixelShaderで使う
	rootParameter[1].Descriptor.ShaderRegister = 1;						//レジスタ番号とバインド
	descriptionRootSignature.pParameters = rootParameter;				//ルートパラメータ配列へのポインタ
	descriptionRootSignature.NumParameters = _countof (rootParameter);	//配列の長さ

	//DescriptorTable
	rootParameter[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;	//DiscriptorTableを使う
	rootParameter[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	//PixelShaderで使う
	rootParameter[2].DescriptorTable.pDescriptorRanges = descriptorRange;	//Tableの中身の配列を指定
	rootParameter[2].DescriptorTable.NumDescriptorRanges = _countof (descriptorRange);	//Tableで利用する数

	//平行光源用のCBV
	rootParameter[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameter[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameter[3].Descriptor.ShaderRegister = 3;

	//InputLayoutの設定
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	//ElementDescをバインド
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof (inputElementDescs);

	//***BlendStateの設定***//
	//すべての色の要素を書き込む
	blendDesc[0].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//共通設定なのでfor文で回す
	for (int i = 1; i < kBlendDescNum; i++) {
		blendDesc[i].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		blendDesc[i].RenderTarget[0].BlendEnable = TRUE;
		blendDesc[i].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc[i].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc[i].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	}

	//---個別の設定---//
	//アルファブレンド
	blendDesc[1].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc[1].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc[1].RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;

	//加算合成
	blendDesc[2].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc[2].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc[2].RenderTarget[0].DestBlend = D3D12_BLEND_ONE;

	//減算合成
	blendDesc[3].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc[3].RenderTarget[0].BlendOp = D3D12_BLEND_OP_SUBTRACT;
	blendDesc[3].RenderTarget[0].DestBlend = D3D12_BLEND_ONE;

	//乗算合成
	blendDesc[4].RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
	blendDesc[4].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc[4].RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;

	//スクリーン合成
	blendDesc[5].RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
	blendDesc[5].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc[5].RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	//--------------//
	//***************//

	//Samplerの設定
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;	//バイリニアフィルタ
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;	//比較しない
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;	//ありったけのmipmapを使う
	staticSamplers[0].ShaderRegister = 0;	//レジスタ番号0を使う
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;		//PixelShaderで使う
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof (staticSamplers);

	//CPU側でRootSignatureの設定をシリアライズしてバイナリにする(GPUが理解できる形に変換する)
	hr = D3D12SerializeRootSignature (&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED (hr)) {
		Log (logStream, reinterpret_cast<char*>(errorBlob->GetBufferPointer ()));
		assert (false);
	}

	//作ったバイナリを元にGPU側にRootSignatureを作成
	hr = device->CreateRootSignature (0, signatureBlob->GetBufferPointer (),
									  signatureBlob->GetBufferSize (), IID_PPV_ARGS (rootSignature.GetAddressOf ()));
	assert (SUCCEEDED (hr));

	//RasterizerStateの設定
	//裏面(時計回り)を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE/*D3D12_CULL_MODE_BACK*/;
	//三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	//Shaderをコンパイルする
	vertexShaderBlob = CompilerShader (L"engine/shader/Object3d.VS.hlsl",
									   L"vs_6_0", dxcUtils.Get (), dxcCompiler.Get (), includeHandler.Get (), logStream);
	assert (vertexShaderBlob != nullptr);

	pixelShaderBlob = CompilerShader (L"engine/shader/Object3d.PS.hlsl",
									  L"ps_6_0", dxcUtils.Get (), dxcCompiler.Get (), includeHandler.Get (), logStream);
	assert (pixelShaderBlob != nullptr);

	//DepthStencilTextureをウィンドウサイズで作成
	depthStencilResource = CreateDepthStencilTextureResource (device.Get (), kClientWidth, kClientHeight);

	//DSVディスクリプタヒープ生成
	dsvDescriptorHeap = CreateDescriptorHeap (device.Get (), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	//DSVの設定
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;		//Format。基本的にはResourceに合わせる
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;		//2dTexture
	//DSVHeapの先頭にDSVを作る
	device->CreateDepthStencilView (depthStencilResource.Get (), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart ());

	//depthStencilStateの設定
	//Depthの機能を有効化する
	depthStencilDesc.DepthEnable = true;
	//書き込みします
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//比較関数はLessEqual。つまり、近ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	//*****実際にPSOを生成する*****//
	graphicsPieplineStateDesc.pRootSignature = rootSignature.Get ();//RootSignature
	graphicsPieplineStateDesc.InputLayout = inputLayoutDesc;		//InputLayout
	graphicsPieplineStateDesc.VS = { vertexShaderBlob->GetBufferPointer (),
	vertexShaderBlob->GetBufferSize () };							//VertexShader
	graphicsPieplineStateDesc.PS = { pixelShaderBlob->GetBufferPointer (),
	pixelShaderBlob->GetBufferSize () };							//PixelShader
	graphicsPieplineStateDesc.BlendState = blendDesc[1];			//BlendState
	graphicsPieplineStateDesc.RasterizerState = rasterizerDesc;		//RastarizerState
	//書き込むRTVの情報
	graphicsPieplineStateDesc.NumRenderTargets = 1;
	graphicsPieplineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//利用するトポロジ(形状)のタイプ。三角形
	graphicsPieplineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//どのように画面に色を打ち込むのかの設定(気にしなくて良い)
	graphicsPieplineStateDesc.SampleDesc.Count = 1;
	graphicsPieplineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	//DepthStencilの設定
	graphicsPieplineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPieplineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	hr = device->CreateGraphicsPipelineState (&graphicsPieplineStateDesc,
											  IID_PPV_ARGS (graphicsPipelineState.GetAddressOf ()));
	assert (SUCCEEDED (hr));
	//***************************//

	//ビューポート設定
	//クライアント領域のサイズと一緒にして画面全体に表示
	viewport.Width = static_cast<float>(kClientWidth);
	viewport.Height = static_cast<float>(kClientHeight);
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	//シザー矩形設定
	//基本的にビューポートと同じ矩形が構成されるようにする
	scissorRect.left = 0;
	scissorRect.right = kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = kClientHeight;

	//------------------------------------------//
	//				ImGuiの初期化					//
	//------------------------------------------//
	IMGUI_CHECKVERSION ();
	ImGui::CreateContext ();
	ImGui::StyleColorsDark ();
	ImGuiIO& io = ImGui::GetIO ();
	ImFont* fontJP = io.Fonts->AddFontFromFileTTF (
		"Resources/AppliMincho/PottaOne-Regular.ttf", 17.0f, nullptr,
		io.Fonts->GetGlyphRangesJapanese ());
	io.FontDefault = fontJP;
	ImGui_ImplWin32_Init (hwnd);
	ImGui_ImplDX12_Init (device.Get (),
						 swapChainDesc.BufferCount,
						 rtvDesc.Format,
						 srvDescriptorHeap.Get (),
						 srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart (),
						 srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart ()
	);
}

void DxCommon::BeginFrame () {
	// //フレームの先頭をImGuiに伝えてあげる
	ImGui_ImplDX12_NewFrame ();
	ImGui_ImplWin32_NewFrame ();
	ImGui::NewFrame ();

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
	/*三角形描画！*/
	commandList->RSSetViewports (1, &viewport);					//Viewportを設定
	commandList->RSSetScissorRects (1, &scissorRect);			//Scissorを設定
	//RootSignatureを設定。PSOに設定しているけど別途設定が必要
	commandList->SetGraphicsRootSignature (rootSignature.Get ());
	commandList->SetPipelineState (graphicsPipelineState.Get ());		//PSOを設定
}

void DxCommon::EndFrame () {
	//ImGuiの内部コマンドを生成する
	ImGui::Render ();
	//実際のImGui描画コマンドを詰む
	ImGui_ImplDX12_RenderDrawData (ImGui::GetDrawData (), commandList.Get ());
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
	//次のフレーム用のコマンドリストを準備
	hr = commandAllocator->Reset ();
	assert (SUCCEEDED (hr));
	hr = commandList->Reset (commandAllocator.Get (), nullptr);
	assert (SUCCEEDED (hr));
}

void DxCommon::Finalize () {
	if (commandList) {
		commandList->Close ();
	}

	// GPUのコマンド完了を確実に待つ
	if (commandQueue && fence) {
		UINT64 currentFenceValue = ++fenceValue;
		commandQueue->Signal (fence.Get (), currentFenceValue);
		if (fence->GetCompletedValue () < currentFenceValue) {
			fence->SetEventOnCompletion (currentFenceValue, fenceEvent);
			WaitForSingleObject (fenceEvent, INFINITE);
		}
	}

	// ImGui終了処理
	ImGui_ImplDX12_Shutdown ();
	ImGui_ImplWin32_Shutdown ();
	ImGui::DestroyContext ();
}