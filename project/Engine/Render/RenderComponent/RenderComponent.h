#pragma once
#include "Component.h"

namespace MyEngine::Rendering {
	// 前方宣言
	class RenderCollector;

	class RenderComponent : public Component {
	public:
		// コンストラクタ
		RenderComponent();

		// 描画コンポーネントリストから自動削除するためのデストラクタ
		virtual ~RenderComponent() override;

		// 
		virtual void CollectRenderData(RenderCollector* collector) const = 0;

		// 表示・非表示の切り替え
		bool IsVisible() const;
		void SetVisible(bool visible);

		// シリアライズ
		void Serialize(json& j) const override;
		void Deserialize(const json& j) override;

	protected:
		// 描画物表示の切り替え
		bool isVisible_ = true;
	};
}