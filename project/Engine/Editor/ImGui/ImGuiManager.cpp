#include "PCH.h"
#include "ImGuiManager.h"
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#include "WindowsAPI.h"
#include "SrvDescriptorHeapPool.h"
#include "EditorManager.h"

ImGuiManager::~ImGuiManager() {
	Finalize();
}

void ImGuiManager::Finalize() {
#ifdef USEIMGUI
	if(ImGui::GetCurrentContext()) {
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	EditorManager::GetInstance()->Finalize();
#endif
}

void ImGuiManager::Initialize(
	ID3D12Device* device,
	ID3D12CommandQueue* cmdQueue,
	MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager
) {
#ifdef USEIMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();

	//タブの上の線を消す
	style.TabBarOverlineSize = 0.0f;

	// メニューバー
	style.Colors[ImGuiCol_MenuBarBg] = { 0.02f, 0.02f, 0.02f, 1.00f };
	style.Colors[ImGuiCol_PopupBg] = { 0.12f, 0.15f, 0.20f, 0.95f };

	// タイトル
	style.Colors[ImGuiCol_TitleBgActive] = { 0.1f, 0.1f, 0.1f, 1.00f };
	style.Colors[ImGuiCol_TitleBg] = { 0.05f, 0.05f, 0.05f, 1.00f };
	style.Colors[ImGuiCol_TitleBgCollapsed] = { 0.05f, 0.05f, 0.05f, 1.00f };

	// タブ
	style.Colors[ImGuiCol_Tab] = { 0.05f, 0.05f, 0.05f, 1.00f };
	style.Colors[ImGuiCol_TabActive] = { 0.15f, 0.15f, 0.15f, 1.00f };
	style.Colors[ImGuiCol_TabHovered] = { 0.1f, 0.1f, 0.1f, 1.00f };
	style.Colors[ImGuiCol_TabUnfocusedActive] = { 0.05f, 0.05f, 0.05f, 1.00f };
	style.Colors[ImGuiCol_TabUnfocused] = { 0.05f, 0.05f, 0.05f, 1.00f };

	// ボタン
	style.Colors[ImGuiCol_Button] = { 0.30f, 0.30f, 0.30f, 1.00f };
	style.Colors[ImGuiCol_ButtonHovered] = { 0.1f, 0.1f, 0.1f, 1.00f };
	style.Colors[ImGuiCol_ButtonActive] = { 0.15f, 0.15f, 0.15f, 1.00f };

	// 選択ハイライト
	style.Colors[ImGuiCol_Header] = { 0.05f, 0.05f, 0.05f, 1.00f };
	style.Colors[ImGuiCol_HeaderHovered] = { 0.1f, 0.1f, 0.1f, 1.00f };
	style.Colors[ImGuiCol_HeaderActive] = { 0.15f, 0.15f, 0.15f, 1.00f };

	ImGuiIO& io = ImGui::GetIO();
	ImFont* fontJP = io.Fonts->AddFontFromFileTTF(
		"Resources/AppliMincho/PottaOne-Regular.ttf", 17.0f, nullptr,
		io.Fonts->GetGlyphRangesJapanese());
	io.FontDefault = fontJP;
	ImGui_ImplWin32_Init(WindowsAPI::GetInstance()->GetHwnd());

	ImGui_ImplDX12_InitInfo initInfo = {};
	initInfo.Device = device;
	initInfo.CommandQueue = cmdQueue;
	initInfo.NumFramesInFlight = 3;
	initInfo.RTVFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	initInfo.SrvDescriptorHeapPool = heapManager->GetHeap();
	initInfo.UserData = heapManager;

	initInfo.SrvDescriptorAllocFn = ImGuiManager::AllocDescriptor;
	initInfo.SrvDescriptorFreeFn = ImGuiManager::FreeDescriptor;

	ImGui_ImplDX12_Init(&initInfo);

	EditorManager::GetInstance()->Initialize();
#endif
}

void ImGuiManager::Draw(ID3D12GraphicsCommandList* cmdList) {
#ifdef USEIMGUI
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
#endif
}

void ImGuiManager::BeginFrame(
	ID3D12Device* device,
	MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager,
	MyEngine::Rendering::RenderTexture* renderTexture
) {
#ifdef USEIMGUI
	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX12_NewFrame();
	ImGui::NewFrame();

	RenderDockingSpace(device, heapManager, renderTexture);
#endif
}

void ImGuiManager::RenderDockingSpace(
	ID3D12Device* device,
	MyEngine::LowLevel::SrvDescriptorHeapPool* heapManager,
	MyEngine::Rendering::RenderTexture* renderTexture
) {
#ifdef USEIMGUI
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoNavFocus;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	window_flags |= ImGuiWindowFlags_MenuBar;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGui::Begin("MyEngineMainDockSpaceWindow", nullptr, window_flags);
	ImGui::PopStyleVar(3);

	ImGuiID dockspace_id = ImGui::GetID("MyEngineDockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

	ImGui::Begin("View");
	ImGui::End();

	EditorManager::GetInstance()->UpdateAndDraw(device, heapManager, renderTexture);

	ImGui::End();
#endif
}

void ImGuiManager::AllocDescriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle) {
	// UserData から heapManager を取り出す
	auto* mgr = static_cast<MyEngine::LowLevel::SrvDescriptorHeapPool*>(info->UserData);

	// コールバック内でインデックスをアロケートする
	uint32_t index = mgr->AllocateIndex();
	*out_cpu_desc_handle = mgr->GetCpuHandle(index);
	*out_gpu_desc_handle = mgr->GetGpuHandle(index);
}

void ImGuiManager::FreeDescriptor(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle) {
	auto* mgr = static_cast<MyEngine::LowLevel::SrvDescriptorHeapPool*>(info->UserData);

	// ハンドルからインデックスを逆引きして解放する
	uint32_t index = mgr->GetIndex(cpu_desc_handle);
	mgr->FreeIndex(index);
}