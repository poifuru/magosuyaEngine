#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <dxgidebug.h>
#include <array>
#include "winAPI/WindowsAPI.h"
#include "../../utility/input/InputManager.h"

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
public:		//メンバ関数(mainで呼び出すよう)
	void Initialize (WindowsAPI* winApp);
	void BeginFrame ();
	void EndFrame ();
	void Finalize ();

	/// <summary>
	/// ディスクリプタヒープ作成関数
	/// </summary>
	/// <param name="device"></param>
	/// <param name="heapType"></param>
	/// <param name="numDescriptors"></param>
	/// <param name="shaderVisible"></param>
	/// <returns></returns>
	ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap (D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

#pragma region ディスクリプタハンドル取得関数(必要になったらRTVやDSVなども)
	/// <summary>
	/// SRV専用のCPUディスクリプタハンドル取得関数
	/// </summary>
	/// <param name="index">要素数</param>
	/// <returns>SRVのCPUディスクリプタハンドル</returns>
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle (uint32_t index);

	/// <summary>
	/// SRV専用のGPUディスクリプタハンドル取得関数
	/// </summary>
	/// <param name="index">要素数</param>
	/// <returns>SRVのGPUディスクリプタハンドル</returns>
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle (uint32_t index);
#pragma endregion
		
private:	//プライベート関数
	void CreateDevice ();
	void CreateCommand ();
	void CreateDxcCompiler ();
	void CreateFence ();
	void CreateDescriptorHeap ();
	void CreateSwapChain();
	void CreateDepthBaffer ();
	void CreateRTV ();
	void CreateDSV ();
	void ViewportRectInit ();
	void ScissorRectInit ();
	void ImGuiInit ();
	//DescriptorHandleを取得する関数(CPUとGPU)
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle (const ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle (const ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t descriptorSize, uint32_t index);

public:		//アクセッサ
	WindowsAPI* GetWinAPI () { return winApp_; }
	ID3D12Device* GetDevice () { return device.Get(); }
	ID3D12GraphicsCommandList* GetCommandList () { return commandList.Get (); }
	ID3D12DescriptorHeap* GetsrvDescriptorHeap () { return srvDescriptorHeap.Get(); }

private://メンバ変数
	D3DResourceLeakChecker leakCheck_{};

	WindowsAPI* winApp_ = nullptr;

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
	std::array<ComPtr<ID3D12Resource>, 2> swapChainResources;

	//RTVを2つ作るのでディスクリプタを2つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2]{};

	//ディスクリプタヒープサイズ
	UINT rtvDescriptorHeapSize_;

	//RTVディスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap{};

	//RTVの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};

	//ディスクリプタヒープサイズ
	UINT srvDescriptorHeapSize_;

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

	//depthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};

	//DepthStencilTexture
	ComPtr<ID3D12Resource> depthStencilResource = nullptr;

	//ディスクリプタヒープサイズ
	UINT dsvDescriptorHeapSize_;

	//DSV用ディスクリプタヒープで数は1。DSVはShader内で触るものではないので、ShaderVisibleはfalse
	ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap{};

	//PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPieplineStateDesc{};
	ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;

	//ビューポート
	D3D12_VIEWPORT viewport{};

	//シザー矩形
	D3D12_RECT scissorRect{};
};