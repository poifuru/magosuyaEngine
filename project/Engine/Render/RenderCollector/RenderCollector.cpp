#include "PCH.h"
#include "RenderCollector.h"

void MyEngine::Rendering::RenderCollector::Submit(const RenderItem& item) {
	items_.push_back(item);
}

const std::vector<MyEngine::Rendering::RenderItem>& MyEngine::Rendering::RenderCollector::GetItems() const {
	return items_;
}

void MyEngine::Rendering::RenderCollector::Clear() {
	items_.clear();
}