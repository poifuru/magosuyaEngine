#pragma once

namespace MyEngine::Rendering {
	// 前方宣言
	struct Mesh;
	class Material;

	// 描画素材を受け取るための構造体
	struct RenderItem {
		const MyEngine::Rendering::Mesh* mesh = nullptr;
		MyEngine::Rendering::Material* material = nullptr;
		D3D12_GPU_VIRTUAL_ADDRESS transformAddr = 0;
		D3D12_GPU_VIRTUAL_ADDRESS customAddr = 0;
		const char* debugName = "Unknown";
	};

	// RenderItemを集めてまとめるクラス
	class RenderCollector {
	public:
		RenderCollector() = default;

		~RenderCollector() = default;

		void Submit(const RenderItem& item);

		const std::vector<RenderItem>& GetItems() const;

		void Clear();

	private:
		std::vector<RenderItem> items_;
	};
}