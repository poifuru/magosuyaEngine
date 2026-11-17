#pragma once
#include "Component.h"

// エミッター構造体
struct Emitter {
	EulerTransform transform;	//transform
	uint32_t count;			//発生数
	float frequency;		//発生頻度
	float frequencyTime;	//頻度用時刻
	bool drawDebug;			//デバッグ用描画
};

// 前方宣言
namespace MyEngine::Rendering {
	class Renderer;
}

struct Particle;
struct ParticleConfig;

class EmitterComponent : public Component {
public:
	EmitterComponent() = default;
	~EmitterComponent() override = default;

	// 基本ライフサイクル
	void Initialize() override;
	void Update() override;
	void ImGui() override;

	// Jsonへの書き出し
	void Serialize(json& j) const override;
	// Jsonからの読み込み
	void Deserialize(const json& j) override;

	// エディタで表示するためのコンポーネント名
	const char* GetName() const override;

private:

};