#pragma once
#include "MathFunction.h"

struct SceneContext;

class ParticleSpawner {
public:
	// 爆発エフェクト
	static void SpawnExplosion(SceneContext* context, const Vector3& position, int count = 10);

	// トレイルエフェクト
	static void SpawnTrail(SceneContext* context, const Vector3& position, const Vector3& direction);

	static void SpawnWaterSplash(SceneContext* context, const Vector3& position, int count = 12);
	static void SpawnBulletGlowTrail(SceneContext* context, const Vector3& position, const Vector3& direction);
};
