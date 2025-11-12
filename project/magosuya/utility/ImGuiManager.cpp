#include "ImGuiManager.h"
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
#include "engine/engineCore/DxCommon.h"

ImGuiManager::ImGuiManager (DxCommon* dxCommon) {
	dxCommon_ = dxCommon;
}

ImGuiManager::~ImGuiManager () {
#ifdef USEIMGUI
	//ImGuiの終了処理
	ImGui_ImplDX12_Shutdown ();
	ImGui_ImplWin32_Shutdown ();
	ImGui::DestroyContext ();
#endif
}

void ImGuiManager::Initialize () {
#ifdef USEIMGUI
	IMGUI_CHECKVERSION ();
	ImGui::CreateContext ();
	ImGui::StyleColorsDark ();
	ImGuiIO& io = ImGui::GetIO ();
	ImFont* fontJP = io.Fonts->AddFontFromFileTTF (
		"Resources/AppliMincho/PottaOne-Regular.ttf", 17.0f, nullptr,
		io.Fonts->GetGlyphRangesJapanese ());
	io.FontDefault = fontJP;
	ImGui_ImplWin32_Init (dxCommon_->GetWinAPI()->GetHwnd ());
	ImGui_ImplDX12_Init (dxCommon_->GetDevice(),
						 2,
						 DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
						 dxCommon_->GetsrvDescriptorHeap(),
						 dxCommon_->GetsrvDescriptorHeap ()->GetCPUDescriptorHandleForHeapStart (),
						 dxCommon_->GetsrvDescriptorHeap ()->GetGPUDescriptorHandleForHeapStart ()
	);
#endif
}

void ImGuiManager::Draw () {
#ifdef USEIMGUI
	//ImGuiの内部コマンドを生成する
	ImGui::Render ();
	//実際のImGui描画コマンドを詰む
	ImGui_ImplDX12_RenderDrawData (ImGui::GetDrawData (), dxCommon_->GetCommandList());
#endif
}

void ImGuiManager::BeginFrame () {
#ifdef USEIMGUI
	//フレームの先頭をImGuiに伝えてあげる
	ImGui_ImplDX12_NewFrame ();
	ImGui_ImplWin32_NewFrame ();
	ImGui::NewFrame ();
#endif
}