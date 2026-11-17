#include "PCH.h"
#include "ParticleSpawner.h"
#include "GameObject.h"
#include "MeshRendererComponent.h"
#include "ParticleComponent.h"
#include "BaseScene.h"
#include "BlendModeManager.h"
#include <cstdlib>
#include <cmath>

void ParticleSpawner::SpawnExplosion(SceneContext* context, const Vector3& position, int count) {
	if (!context || !context->gameObjects) return;

	for (int i = 0; i < count; ++i) {
		auto partObj = std::make_unique<GameObject>(context, "Particle");
		partObj->SetSerializable(false); // セーブ対象から除外

		auto* mesh = partObj->AddComponent<MyEngine::Rendering::MeshRendererComponent>();
		auto* partComp = partObj->AddComponent<ParticleComponent>();

		partObj->GetTransform().translate = position;
		// スケールを大きくする（0.3f 〜 0.7f）
		float scaleSize = 0.3f + static_cast<float>(rand()) / RAND_MAX * 0.4f;
		partObj->GetTransform().scale = { scaleSize, scaleSize, scaleSize };

		// 先に初期化する（これでデフォルト値が一度セットされる）
		partObj->Initialize();

		// 初期化された後に、モデルや色、ライティング設定を上書きする！（リセット防止）
		mesh->SetModel("Resources/plane/plane.obj");
		mesh->SetTexture("Resources/ParticleAssets/circle2.png");
		mesh->SetEnableLighting(false);
		mesh->SetBlendMode(MyEngine::Rendering::BlendModeType::Additive);
		mesh->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		mesh->SetDoubleSided(true);

		float theta = static_cast<float>(rand()) / RAND_MAX * 3.14159265f * 2.0f;
		float phi = static_cast<float>(rand()) / RAND_MAX * 3.14159265f;
		float speed = 2.0f + static_cast<float>(rand()) / RAND_MAX * 6.0f;

		Vector3 velocity = {
			speed * std::sin(phi) * std::cos(theta),
			speed * std::cos(phi),
			speed * std::sin(phi) * std::sin(theta)
		};

		partComp->SetVelocity(velocity);
		partComp->SetGravity(9.8f);
		partComp->SetLifeTime(0.3f + static_cast<float>(rand()) / RAND_MAX * 0.4f);
		partComp->SetColorFade({ 1.0f, 0.6f, 0.1f, 1.0f }, { 1.0f, 0.0f, 0.0f, 0.0f });

		// 生成された最初のフレームの描画前に確実にGPUにデータを送るため、手動でUpdateを走らせる！
		partObj->Update();

		context->gameObjects->push_back(std::move(partObj));
	}
}

void ParticleSpawner::SpawnTrail(SceneContext* context, const Vector3& position, const Vector3& direction) {
	if (!context || !context->gameObjects) return;

	auto partObj = std::make_unique<GameObject>(context, "Particle");
	partObj->SetSerializable(false);

	auto* mesh = partObj->AddComponent<MyEngine::Rendering::MeshRendererComponent>();
	auto* partComp = partObj->AddComponent<ParticleComponent>();

	// 水面に沈んで隠れないよう、最小高度 y=0.25f に浮上補正
	Vector3 spawnPos = position;
	if (spawnPos.y < 0.25f) {
		spawnPos.y = 0.25f;
	}
	partObj->GetTransform().translate = spawnPos;

	// スケールを大きくして船の引き波を見やすく表示（1.2f）
	float scaleSize = 1.2f + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.4f;
	partObj->GetTransform().scale = { scaleSize, scaleSize, scaleSize };

	// 先に初期化する
	partObj->Initialize();

	// 初期化された後に、モデルや色、ライティング設定を上書きする！（リセット防止）
	mesh->SetModel("Resources/plane/plane.obj");
	mesh->SetTexture("Resources/ParticleAssets/circle2.png");
	mesh->SetEnableLighting(false);
	mesh->SetBlendMode(MyEngine::Rendering::BlendModeType::Alpha);
	mesh->SetColor({ 1.0f, 1.0f, 1.0f, 0.75f });
	mesh->SetDoubleSided(true);

	float randX = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f;
	float randY = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.5f;
	float randZ = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f;
	Vector3 velocity = {
		-direction.x * 1.5f + randX,
		-direction.y * 1.5f + randY + 0.2f,
		-direction.z * 1.5f + randZ
	};

	partComp->SetVelocity(velocity);
	partComp->SetGravity(0.0f);
	partComp->SetLifeTime(0.6f);
	partComp->SetColorFade({ 1.0f, 1.0f, 1.0f, 0.75f }, { 1.0f, 1.0f, 1.0f, 0.0f });

	// 生成された最初のフレームの描画前に確実にGPUにデータを送るため、手動でUpdateを走らせる！
	partObj->Update();

	context->gameObjects->push_back(std::move(partObj));
}

void ParticleSpawner::SpawnWaterSplash(SceneContext* context, const Vector3& position, int count) {
	if (!context || !context->gameObjects) return;

	for (int i = 0; i < count; ++i) {
		auto partObj = std::make_unique<GameObject>(context, "Particle");
		partObj->SetSerializable(false);

		auto* mesh = partObj->AddComponent<MyEngine::Rendering::MeshRendererComponent>();
		auto* partComp = partObj->AddComponent<ParticleComponent>();

		partObj->GetTransform().translate = position;
		float scaleSize = 0.6f + (static_cast<float>(rand()) / RAND_MAX) * 1.0f;
		partObj->GetTransform().scale = { scaleSize, scaleSize, scaleSize };

		partObj->Initialize();

		mesh->SetModel("Resources/plane/plane.obj");
		mesh->SetTexture("Resources/ParticleAssets/circle2.png");
		mesh->SetEnableLighting(false);
		mesh->SetBlendMode(MyEngine::Rendering::BlendModeType::Additive);
		mesh->SetColor({ 0.8f, 0.95f, 1.0f, 1.0f });
		mesh->SetDoubleSided(true);

		float theta = (static_cast<float>(rand()) / RAND_MAX) * 3.14159265f * 2.0f;
		float speedHoriz = 1.0f + (static_cast<float>(rand()) / RAND_MAX) * 4.0f;
		float speedUp = 5.0f + (static_cast<float>(rand()) / RAND_MAX) * 8.0f;

		Vector3 velocity = {
			speedHoriz * std::cos(theta),
			speedUp,
			speedHoriz * std::sin(theta)
		};

		partComp->SetVelocity(velocity);
		partComp->SetGravity(18.0f); // 重力で水飛沫が落ちる
		partComp->SetLifeTime(0.5f + (static_cast<float>(rand()) / RAND_MAX) * 0.3f);
		partComp->SetColorFade({ 0.8f, 0.95f, 1.0f, 1.0f }, { 0.2f, 0.6f, 1.0f, 0.0f });

		partObj->Update();

		context->gameObjects->push_back(std::move(partObj));
	}
}

void ParticleSpawner::SpawnBulletGlowTrail(SceneContext* context, const Vector3& position, const Vector3& direction) {
	if (!context || !context->gameObjects) return;

	auto partObj = std::make_unique<GameObject>(context, "Particle");
	partObj->SetSerializable(false);

	auto* mesh = partObj->AddComponent<MyEngine::Rendering::MeshRendererComponent>();
	auto* partComp = partObj->AddComponent<ParticleComponent>();

	partObj->GetTransform().translate = position;
	// 加算発光するスマートなスリム粒子のトレイル（0.6f）
	float scaleSize = 0.6f;
	partObj->GetTransform().scale = { scaleSize, scaleSize, scaleSize };

	partObj->Initialize();

	mesh->SetModel("Resources/plane/plane.obj");
	mesh->SetTexture("Resources/ParticleAssets/circle2.png");
	mesh->SetEnableLighting(false);
	mesh->SetBlendMode(MyEngine::Rendering::BlendModeType::Additive); // 加算合成で水面透過
	mesh->SetColor({ 0.3f, 0.8f, 1.0f, 1.0f }); // 眩しい水色発光
	mesh->SetDoubleSided(true);

	Vector3 velocity = {
		-direction.x * 0.5f,
		-direction.y * 0.5f,
		-direction.z * 0.5f
	};

	partComp->SetVelocity(velocity);
	partComp->SetGravity(0.0f);
	partComp->SetLifeTime(0.35f);
	partComp->SetColorFade({ 0.3f, 0.8f, 1.0f, 1.0f }, { 0.0f, 0.2f, 1.0f, 0.0f });

	partObj->Update();

	context->gameObjects->push_back(std::move(partObj));
}
