#pragma once
#include "Component.h"
#include "MathTypes.h"
#include "GpuParticleSystem.h"   // ParticleEmitTimer をメンバに持つので include する
#include <string>

// GPU パーティクルのエフェクト（Resources/Particle/<名前>.json）を、このオブジェクトの位置から出すコンポーネント
// ・出し続けるエフェクト：Play 中は毎フレーム、そのフレームの分を Emit する
// ・一発物のエフェクト：Play を呼んだときに1回 Burst する
class GpuEmitterComponent : public Component {
public:
	GpuEmitterComponent() = default;
	~GpuEmitterComponent() override = default;

	void Initialize() override;
	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "GpuEmitterComponent"; }

	// ゲームから呼ぶ
	void Play();   // 出し続ける物は出し始める。一発物は1回バーストする
	void Stop();   // 出し続けるのをやめる（飛んでいる粒はそのまま消えていく）
	bool IsPlaying() const { return isPlaying_; }

	void SetEffectName(const std::string& name) { effectName_ = name; }
	const std::string& GetEffectName() const { return effectName_; }

private:
	GpuParticleSystem* GetSystem() const;   // gameObject_ → context → renderer → System
	Vector3 GetEmitPosition() const;        // オブジェクトの位置 + offset_

private:
	std::string effectName_ = "fire";
	Vector3 offset_ = { 0.0f, 0.0f, 0.0f };   // オブジェクトの位置からのずらし
	bool playOnStart_ = true;                 // 初期化のときに Play する
	bool isPlaying_ = false;
	ParticleEmitTimer timer_;
};
