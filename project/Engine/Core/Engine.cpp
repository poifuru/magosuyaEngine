#include "PCH.h"
#include "Engine.h"
#include "WindowsAPI.h"
#include "InputManager.h"
#include "FrameRateController.h"
#include "GraphicsDevice.h"
#include "DxcCompiler.h"
#include "CommandQueue.h"
#include "CommandList.h"
#include "SwapChain.h"
#include "SrvDescriptorHeapPool.h"
#include "Logger.h"
#include "RenderTexture.h"

// プロファイラ用の静的変数定義
float MyEngine::LowLevel::Engine::sUpdateTime_ = 0.0f;
float MyEngine::LowLevel::Engine::sRenderTime_ = 0.0f;
float MyEngine::LowLevel::Engine::sGpuWaitTime_ = 0.0f;

MyEngine::LowLevel::Engine::Engine() = default;
MyEngine::LowLevel::Engine::~Engine() {
	WindowsAPI::GetInstance()->Finalize();
}

void MyEngine::LowLevel::Engine::Initialize() {
	WindowsAPI::GetInstance()->Initialize(1280, 720);
	WindowsAPI::GetInstance()->RegisterEngine(this);

	Logger::GetInstance()->Initialize();

	InputManager::GetInstance()->Initialize(WindowsAPI::GetInstance()->GetHwnd());

	frameRateController_ = MyEngine::LowLevel::FrameRateController::GetInstance();

	device_ = std::make_unique<MyEngine::LowLevel::GraphicsDevice>();
	device_->Initialize();

	dxcCompiler_ = std::make_unique<MyEngine::LowLevel::DxcCompiler>();
	dxcCompiler_->Initialize();

	cmdQueue_ = std::make_unique<MyEngine::LowLevel::CommandQueue>();
	cmdQueue_->Initialize(device_->GetDevice());

	cmdList_ = std::make_unique<MyEngine::LowLevel::CommandList>();
	cmdList_->Initialize(device_->GetDevice());

	swapChain_ = std::make_unique<MyEngine::LowLevel::SwapChain>();
	swapChain_->Initialize(
		device_->GetDxgiFactory(),
		cmdQueue_->GetCommandQueue(),
		WindowsAPI::GetInstance()->GetHwnd(),
		WindowsAPI::GetInstance()->GetWindowWidth(),
		WindowsAPI::GetInstance()->GetWindowHeight()
	);

	heapManager_ = std::make_unique<MyEngine::LowLevel::SrvDescriptorHeapPool>();
	heapManager_->Initialize(device_->GetDevice(), 4096);

	swapChain_->CreateDepthSRV(device_->GetDevice(), heapManager_.get());
}

bool MyEngine::LowLevel::Engine::ProcessMessage() {
	return WindowsAPI::GetInstance()->ProcessMessage();
}

void MyEngine::LowLevel::Engine::BeginFrame(D3D12_CPU_DESCRIPTOR_HANDLE renderTexDescriptorHandle) {
	// 予約された遅延リサイズを安全に実行する
	if (resizeRequested_) {
		if (swapChain_) {
			cmdQueue_->SignalAndWait();
			swapChain_->Resize(newWidth_, newHeight_);
		}
		resizeRequested_ = false;
	}

	frameRateController_->Update();
	InputManager::GetInstance()->Update();
	cmdList_->Reset();

#ifdef USEIMGUI
	/*D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = swapChain_->GetDsvHandle();
	cmdList_->SetRenderTargets(renderTexDescriptorHandle, &dsvHandle);*/
#else
	// 直接SwapChainのバックバッファに描画する
	float clearColor[] = { 0.14f, 0.14f, 0.14f, 1.0f };
	swapChain_->BeginRender(cmdList_.get(), clearColor);
#endif
}

void MyEngine::LowLevel::Engine::EndFrame() {

	swapChain_->EndRender(cmdList_.get());
	cmdQueue_->ExecuteCommandList(cmdList_->GetCommandList());
	swapChain_->Present();
	cmdQueue_->SignalAndWait();

	InputManager::GetInstance()->EndFrame();
}

void MyEngine::LowLevel::Engine::BeginSwapChainRender() {
#ifdef USEIMGUI
	// SwapChainの準備 (ImGuiの描画先)
	float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
	swapChain_->BeginRender(cmdList_.get(), clearColor);
#endif
}

void MyEngine::LowLevel::Engine::ResetCommandList() {
	cmdList_->Reset();
}

void MyEngine::LowLevel::Engine::ExecuteCommandList() {
	cmdQueue_->ExecuteCommandList(cmdList_->GetCommandList());
	cmdQueue_->SignalAndWait();
}

void MyEngine::LowLevel::Engine::OnResize(uint32_t width, uint32_t height) {
	// 即座に実行せず、リサイズを予約する
	resizeRequested_ = true;
	newWidth_ = width;
	newHeight_ = height;
}

ID3D12Device* MyEngine::LowLevel::Engine::GetDevice() {
	return device_->GetDevice();
}

MyEngine::LowLevel::GraphicsDevice* MyEngine::LowLevel::Engine::GetGraphicsDevice() {
	return device_.get();
}

ID3D12GraphicsCommandList* MyEngine::LowLevel::Engine::GetCommandList() {
	return cmdList_->GetCommandList();;
}

ID3D12CommandQueue* MyEngine::LowLevel::Engine::GetCommandQueue() {
	return cmdQueue_->GetCommandQueue();
}

IDxcUtils* MyEngine::LowLevel::Engine::GetDxcUtils() {
	return dxcCompiler_->GetDxcUtils();
}

IDxcCompiler3* MyEngine::LowLevel::Engine::GetDxcCompiler() {
	return dxcCompiler_->GetDxcCompiler();
}

IDxcIncludeHandler* MyEngine::LowLevel::Engine::GetIncludeHandler() {
	return dxcCompiler_->GetIncludeHandler();
}
