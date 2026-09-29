#pragma once
#include <memory>
#include "BasePostEffect.h"

namespace MyEngine::LowLevel {
	class SrvDescriptorHeap;
}

namespace MyEngine::Rendering {
	class RenderTexture;
}

class CopyImageEffect;
class CameraOrganizer;

// ポストエフェクトの種類
enum class PostEffectType {
	CopyImage,
	Outline,
	ColorGrading,
	Fog,
	Vignette,
	RadialBlur,
	Dissolve,
	RandomNoise,

	Count	// エフェクトの総数
};

class PostEffectManager {
public:
	void Initialize(ID3D12Device* device);
	void SetEffectActive(PostEffectType type, bool flag);
	void ClearEffects();
	void ImGui();

	// Renderer がアクセスできるようにゲッターを用意
	size_t GetEffectCount() const { return static_cast<size_t>(PostEffectType::Count); }
	BasePostEffect* GetEffect(size_t index) const { return effects_[index].get(); }

	// 各エフェクトにアクセスするため
	template <typename T>
	T* GetEffect(PostEffectType type) {
		return dynamic_cast<T*>(GetEffect(static_cast<size_t>(type)));
	}

private:
	// エフェクトの総数で固定管理する
	std::unique_ptr<BasePostEffect> effects_[static_cast<size_t>(PostEffectType::Count)];
};