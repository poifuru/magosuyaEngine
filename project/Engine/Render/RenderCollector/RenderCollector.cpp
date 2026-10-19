#include "PCH.h"
#include "RenderCollector.h"

void MyEngine::Rendering::RenderCollector::Submit(const RenderItem& item) {
	items_.push_back(item);
}

void MyEngine::Rendering::RenderCollector::Submit(
	const MyEngine::Rendering::Mesh& mesh,
	MyEngine::Rendering::Material* material,
	D3D12_GPU_VIRTUAL_ADDRESS transformAddr,
	const char* debugName,
	D3D12_GPU_VIRTUAL_ADDRESS customAddr
) {
	RenderItem item{};
	item.mesh = &mesh;
	item.material = material;
	item.transformAddr = transformAddr;
	item.customAddr = customAddr;
	item.debugName = debugName;
	items_.push_back(item);
}

const std::vector<MyEngine::Rendering::RenderItem>& MyEngine::Rendering::RenderCollector::GetItems() const {
	return items_;
}

void MyEngine::Rendering::RenderCollector::Clear() {
	items_.clear();
}