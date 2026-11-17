#pragma once

// パーティクル一粒ごとの構造体
struct Particle {
	EulerTransform transform;	// トランスフォーム
	Vector3 velocity;			// 速度
	Vector3 acceleration;		// 加速度
	Vector4 color;				// 色
	float currentLifeTime;		// 生まれてからの時間
	float maxLifeTime;			// 寿命
	bool isActive = false;		// 有効か
};

// パーティクルの保存項目
struct ParticleConfig {
	uint32_t maxParticles;
	float spawnRate;
	float minLifeTime;
	float maxLifeTime;
	Vector3 minVelocity;
	Vector3 maxVelocity;
	Vector3 acceleration;
	Vector4 color;
	Vector3 scale;
	std::string texturePath;
};

struct ParticleVertex {
	Vector4 position;
	Vector2 texcoord;
};

struct ParticleForGPU {
	Matrix4x4 world;
	Matrix4x4 WVP;
	Vector4 color;
};