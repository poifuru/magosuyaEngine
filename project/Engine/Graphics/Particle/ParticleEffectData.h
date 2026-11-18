#pragma once
#include "RenderingModel.h"

struct ParticleEffectData {
	int version = 1;
	uint32_t maxCount = 1024;      // 粒の最大数（バッファの大きさ）

	// 放出
	bool loop = true;              // true: rate で出し続ける / false: Burst したときだけ
	float rate = 300.0f;           // 1秒あたりの数（loop のとき）
	uint32_t burstCount = 100;     // Burst 1回で出す数

	// 形
	uint32_t shape = 1;            // 0:点 1:球 2:箱 3:円錐
	float radius = 0.3f;
	Vector3 boxSize = { 1.0f, 1.0f, 1.0f };
	Vector3 direction = { 0.0f, 1.0f, 0.0f };
	float spreadAngle = 30.0f;     // 度

	// 寿命と速さ（x:下限 y:上限）
	Vector2 lifeTime = { 1.0f, 2.0f };
	Vector2 speed = { 4.0f, 8.0f };

	// 力
	Vector3 acceleration = { 0.0f, -9.8f, 0.0f };
	float drag = 0.0f;

	// 見た目
	Vector4 startColor = { 1.0f, 0.6f, 0.1f, 1.0f };
	Vector4 endColor = { 1.0f, 0.0f, 0.0f, 0.0f };
	Vector2 startSize = { 0.3f, 0.7f };
	float endSize = 0.2f;
	std::string texture = "Resources/ParticleAssets/circle2.png";
	MyEngine::Rendering::BlendModeType blend = MyEngine::Rendering::BlendModeType::Additive;
};

bool LoadParticleEffect(const std::string& path, ParticleEffectData& out);
bool SaveParticleEffect(const std::string& path, const ParticleEffectData& data);