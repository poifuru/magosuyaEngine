#pragma once

// エミッターの構造体
struct EmitterForGPU {
	Vector3 position;		// 位置
	uint32_t emitCount;     // 出現させる数

	Vector3 direction;		// 向き
	float spreadAngle;		// 放出方向

	Vector3 boxSize;		// エミッター自体の大きさ
	uint32_t shape;         // 0:点 1:球 2:箱 3:円錐

	Vector3 acceleration;	// 加速度
	float drag;				// 重力や空気抵抗

	Vector4 startColor;		// 出現したときの色

	Vector4 endColor;		// 消滅するときの色

	Vector2 lifeTime;		// 寿命
	Vector2 speed;			// スピードの範囲(x:下限 y:上限)

	Vector2 startSize;		// 出現したときの大きさ
	float endSize;			// 消滅するときの大きさ
	float radius;			// 半径

	float deltaTime;		// デルタタイム
	uint32_t randomSeed;    // ランダムシード
	uint32_t maxCount;      // 最大数
	float padding;
};
static_assert(sizeof(EmitterForGPU) == 144);

// パーティクルの構造体
struct GpuParticle {
	Vector3 position;
	float age;

	Vector3 velocity;
	float lifeTime;

	Vector4 color;

	float size;
	float rotation;
	float rotationSpeed;
	float alive;
};
static_assert(sizeof(GpuParticle) == 64);

// パーティクル描画用の構造体
struct ParticleDrawForGpu {
	Matrix4x4 viewProjection;

	Vector3 cameraRight;
	float size;

	Vector3 cameraUp;
	float padding;

	Vector4 startColor;

	Vector4 endColor;

	float endSize;        // 消えるときの倍率
	float padding2[3];
};
static_assert(sizeof(ParticleDrawForGpu) == 144);