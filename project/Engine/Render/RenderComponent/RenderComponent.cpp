#include "PCH.h"
#include "RenderComponent.h"
#include "GameObject.h"

MyEngine::Rendering::RenderComponent::RenderComponent() {

}

MyEngine::Rendering::RenderComponent::~RenderComponent() {
	// 破棄したときに自動でリストから抜ける
	if(gameObject_ && gameObject_->GetContext()) {
		if(auto* renderer = gameObject_->GetContext()->renderer) {
			renderer->UnregisterRenderComponent(this);
		}
	}
}

bool MyEngine::Rendering::RenderComponent::IsVisible() const {
	return isVisible_;
}

void MyEngine::Rendering::RenderComponent::SetVisible(bool visible) {
	isVisible_ = visible;
}

void MyEngine::Rendering::RenderComponent::Serialize(json& j) const {
}

void MyEngine::Rendering::RenderComponent::Deserialize(const json& j) {
}
