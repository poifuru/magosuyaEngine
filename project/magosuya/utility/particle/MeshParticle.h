#pragma once
#include <vector>
#include <list>
#include <memory>
#include <random>
#include "struct.h"
#include "Mesh.h"

struct MeshParticleData {
	CubeData cube;
	Vector3 velocity;
	float lifeTime;
	float currentTime;
};

class MeshParticle {
public:
	MeshParticle ();
	~MeshParticle ();

	void Initialize ();
	void Update (Matrix4x4* vp);
	void Draw ();
	void ImGui ();

	uint32_t GetParticleNum () { return kMaxParticleNum_; };

private:	//内部関数
	MeshParticleData MakeNewParticle (std::mt19937 randomEngine, const Emitter& emitter_);
	std::list<MeshParticleData> Emit (const Emitter& emitter, std::mt19937& randomEngine);
	void EmitterUpdate ();

public:
	const uint32_t kMaxParticleNum_ = 5000;
	uint32_t numInstance_ = 0; //描画すべきインスタンス数

	//位置データ
	std::list<MeshParticleData> particles_;
	std::list<MeshParticleData>::iterator particleIterator_;
	Emitter emitter_ = {};

	//vp行列
	Matrix4x4* vp_ = {};

	//速度をランダムに割り当てるための乱数生成器
	//生成エンジンの型を作る
	using RNG_Engine = std::mt19937;
	//作った型でエンジンの宣言
	RNG_Engine randomEngine_;
	//実行ごとに異なる値を取得する
	std::random_device rd;
	//分布
	std::uniform_real_distribution<float> pos_x;			//発生位置_x
	std::uniform_real_distribution<float> pos_y;			//発生位置_y
	std::uniform_real_distribution<float> pos_z;			//発生位置_z
	std::uniform_real_distribution<float> randRotate_;		//サイズ
	std::uniform_real_distribution<float> randSize_;		//サイズ
	std::uniform_real_distribution<float> randVelocity_;	//速度
	std::uniform_real_distribution<float> randColor_;		//色
	std::uniform_real_distribution<float> randTime_;		//パーティクルの生存可能時間
};