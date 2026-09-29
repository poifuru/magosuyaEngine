#pragma once

namespace MyEngine::LowLevel {
	class SrvDescriptorHeap;
}

namespace MyEngine::Rendering {
	class RenderTexture;
}

class ImGuiManager {
public:
	static ImGuiManager* GetInstance() {
		static ImGuiManager instance;
		return &instance;
	}
	~ImGuiManager();

	void Initialize(
		ID3D12Device* device,
		ID3D12CommandQueue* cmdQueue,
		MyEngine::LowLevel::SrvDescriptorHeap* heapManager
	);

	void Finalize();

	void Draw(ID3D12GraphicsCommandList* cmdList);

	void BeginFrame(
		ID3D12Device* device,
		MyEngine::LowLevel::SrvDescriptorHeap* heapManager,
		MyEngine::Rendering::RenderTexture* renderTexture
	);

private:
	void RenderDockingSpace(
		ID3D12Device* device,
		MyEngine::LowLevel::SrvDescriptorHeap* heapManager,
		MyEngine::Rendering::RenderTexture* renderTexture
	);

	// コールバック用の静的メンバ関数
	static void AllocDescriptor(
		ImGui_ImplDX12_InitInfo* info, 
		D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, 
		D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle
	);
	static void FreeDescriptor(
		ImGui_ImplDX12_InitInfo* info, 
		D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, 
		D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle
	);

private:
	ImGuiManager() = default;
	ImGuiManager(const ImGuiManager&) = delete;
	ImGuiManager& operator=(const ImGuiManager&) = delete;
	ImGuiManager(ImGuiManager&&) = delete;
	ImGuiManager& operator=(ImGuiManager&&) = delete;
};

