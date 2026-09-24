#include "PCH.h"
#include "PerformanceWindow.h"
#include "Engine.h"
#include "RenderSystem.h"

PerformanceWindow::PerformanceWindow() : IEditorWindow("パフォーマンス", true) {}

void PerformanceWindow::Initialize() {
	IEditorWindow::Initialize();
}

void PerformanceWindow::UpdateAndDraw(const EditorContext& context) {
#ifdef USEIMGUI
	// 閉じていたらスキップ
	if(!isOpen_) return;

	// isOpenを渡してxボタンと対応させる
	if(ImGui::Begin(name_.c_str(), &isOpen_)) {
		// FPSとCPU時間の表示
		float fps = ImGui::GetIO().Framerate;
		float frameTimeMs = 1000.0f / fps;
		ImGui::Text("FPS: %.1f", fps);
		ImGui::Text("Frame Time: %.3f ms", frameTimeMs);

		// FPSの簡易グラフ表示
		static float fpsHistory[120] = {};
		static int offset = 0;
		fpsHistory[offset] = fps;
		offset = (offset + 1) % 120;
		ImGui::PlotLines("FPS History", fpsHistory, 120, offset, nullptr, 0.0f, 120.0f, ImVec2(0, 50));
		ImGui::Separator();

		// ドローコール数とメインメモリ使用量の表示
		ImGui::Text("Draw Calls: %u", MyEngine::Rendering::RenderSystem::GetDrawCallCount());

		PROCESS_MEMORY_COUNTERS memCounter;
		GetProcessMemoryInfo(GetCurrentProcess(), &memCounter, sizeof(memCounter));
		float ramUsageMB = static_cast<float>(memCounter.WorkingSetSize) / (1024.0f * 1024.0f);
		ImGui::Text("RAM Usage: %.1f MB", ramUsageMB);
		ImGui::Separator();

		// GPU情報とVRAM使用量の取得 (DirectX 12 / DXGI)
		Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
		if (context.device && SUCCEEDED(context.device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)))) {
			Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
			if (SUCCEEDED(dxgiDevice->GetAdapter(&dxgiAdapter))) {

				// GPUのグラフィックボード名を取得して表示
				DXGI_ADAPTER_DESC desc;
				dxgiAdapter->GetDesc(&desc);
				std::wstring wName(desc.Description);
				std::string gpuName;
				gpuName.reserve(wName.size());
				for (wchar_t w : wName) {
					gpuName.push_back(static_cast<char>(w));
				}
				ImGui::Text("GPU: %s", gpuName.c_str());

				// VRAM（ビデオメモリ）の使用状況を取得 (IDXGIAdapter3へキャストが必要)
				Microsoft::WRL::ComPtr<IDXGIAdapter3> dxgiAdapter3;
				if (SUCCEEDED(dxgiAdapter.As(&dxgiAdapter3))) {
					DXGI_QUERY_VIDEO_MEMORY_INFO memoryInfo{};
					dxgiAdapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memoryInfo);

					// バイトからMBに変換
					float usageMB = static_cast<float>(memoryInfo.CurrentUsage) / (1024.0f * 1024.0f);
					float budgetMB = static_cast<float>(memoryInfo.Budget) / (1024.0f * 1024.0f);

					ImGui::Text("VRAM: %.1f MB / %.1f MB", usageMB, budgetMB);

					// メモリ使用率のプログレスバー
					ImGui::ProgressBar(usageMB / budgetMB, ImVec2(0.0f, 0.0f));
				}
			}
		}
		ImGui::Separator();

		// CPUプロファイラ時間の表示
		ImGui::Text("CPU Profiler");
		float updateTime = MyEngine::LowLevel::Engine::GetUpdateTime();
		float renderTime = MyEngine::LowLevel::Engine::GetRenderTime();
		float waitTime = MyEngine::LowLevel::Engine::GetGpuWaitTime();
		float totalFrameTime = updateTime + renderTime + waitTime;

		// 割合の計算
		float updateRatio = totalFrameTime > 0.0f ? updateTime / totalFrameTime : 0.0f;
		float renderRatio = totalFrameTime > 0.0f ? renderTime / totalFrameTime : 0.0f;
		float waitRatio   = totalFrameTime > 0.0f ? waitTime / totalFrameTime : 0.0f;

		// Update (Game)
		ImGui::Text("Update (Game): %.3f ms", updateTime);
		ImGui::ProgressBar(updateRatio, ImVec2(-60.0f, 0.0f), "");
		ImGui::SameLine();
		ImGui::Text("%.0f%%", updateRatio * 100.0f);

		// Render (CPU)
		ImGui::Text("Render (CPU): %.3f ms", renderTime);
		ImGui::ProgressBar(renderRatio, ImVec2(-60.0f, 0.0f), "");
		ImGui::SameLine();
		ImGui::Text("%.0f%%", renderRatio * 100.0f);

		// GPU Wait
		ImGui::Text("GPU Wait: %.3f ms", waitTime);
		ImGui::ProgressBar(waitRatio, ImVec2(-60.0f, 0.0f), "");
		ImGui::SameLine();
		ImGui::Text("%.0f%%", waitRatio * 100.0f);
	}
	ImGui::End();
#endif
}