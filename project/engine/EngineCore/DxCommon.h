#pragma once
#include "../../header/Engine.h"
#include "../Input/InputManager.h"
#include "winAPI/WindowsAPI.h"
#include "../../header/pch.h"

//BlendStateの個数
const int kBlendDescNum = 6;

struct D3DResourceLeakChecker {
	~D3DResourceLeakChecker () {
		ComPtr<IDXGIDebug> debug;
		if (SUCCEEDED (DXGIGetDebugInterface1 (0, IID_PPV_ARGS (debug.GetAddressOf ())))) {
			debug->ReportLiveObjects (DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects (DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects (DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		}
	}
};

class DxCommon {
public:	//メンバ関数(mainで呼び出すよう)
	void Initialize ();
	void BeginFrame ();
	void EndFrame ();
	void Finalize ();

	//アクセッサ
	WindowsAPI* GetWinAPI () { return winApi_.get(); }
	ID3D12Device* GetDevice () { return device.Get(); }
	ID3D12GraphicsCommandList* GetCommandList () { return commandList.Get (); }
	ID3D12DescriptorHeap* GetsrvDescriptorHeap () { return srvDescriptorHeap.Get(); }

private://メンバ変数
	D3DResourceLeakChecker leakCheck_{};

	std::unique_ptr<WindowsAPI> winApi_;

	//***DX12変数***//
	//DXGIファクトリー
	ComPtr<IDXGIFactory7> dxgiFactory = nullptr;

	//使用するアダプタ用の変数。最初にnullptrを入れておく
	ComPtr<IDXGIAdapter4> useAdapter = nullptr;

	//dxcCompiler
	ComPtr<IDxcUtils> dxcUtils = nullptr;
	ComPtr<IDxcCompiler3> dxcCompiler = nullptr;

	//後のincludeに対応するための設定
	ComPtr<IDxcIncludeHandler> includeHandler = nullptr;

	//デバイス
	ComPtr<ID3D12Device> device = nullptr;

	//コマンドキュー
	ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};

	//コマンドアロケータ
	ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;

	//コマンドリスト
	ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;

	//スワップチェーン
	ComPtr<IDXGISwapChain4> swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};

	//swapChainResource
	ComPtr<ID3D12Resource> swapChainResources[2] = { nullptr };

	//RTVを2つ作るのでディスクリプタを2つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[5]{};

	//RTVディスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap{};

	//RTVの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};

	//SRVディスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap{};

	//初期値0でFenceを作る
	ComPtr<ID3D12Fence> fence = nullptr;
	uint64_t fenceValue = 0;

	//FenceのSignalを待つためのイベント
	HANDLE fenceEvent;

	//DiscriptorRange
	D3D12_DESCRIPTOR_RANGE descriptorRange[1]{};

	//RootSignature
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	//RootParameter
	D3D12_ROOT_PARAMETER rootParameter[4]{};

	//InputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};

	//BlendState(何個か作る)
	D3D12_BLEND_DESC blendDesc[kBlendDescNum]{};

	//Sampler
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1]{};

	//RootSignatureの設定をシリアライズしてバイナリにする
	ComPtr<ID3DBlob> signatureBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	ComPtr<ID3D12RootSignature> rootSignature = nullptr;

	//RasterizerState
	D3D12_RASTERIZER_DESC rasterizerDesc{};

	//Shader
	ComPtr<IDxcBlob> vertexShaderBlob = nullptr;
	ComPtr<IDxcBlob> pixelShaderBlob = nullptr;

	//DepthStencilTexture
	ComPtr<ID3D12Resource> depthStencilResource = nullptr;

	//DSV用ディスクリプタヒープで数は1。DSVはShader内で触るものではないので、ShaderVisibleはfalse
	ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap{};

	//DSVの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};

	//depthStencilState
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};

	//PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPieplineStateDesc{};
	ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;

	//ビューポート
	D3D12_VIEWPORT viewport{};

	//シザー矩形
	D3D12_RECT scissorRect{};
};