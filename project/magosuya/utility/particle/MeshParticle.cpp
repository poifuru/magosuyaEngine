#include "MeshParticle.h"
#include <imgui.h>
#include <numbers>
#include "MathFunction.h"

//デルタタイムを定義
const float kDeltaTime = 1.0f / 60.0f;

MeshParticle::MeshParticle () {
	//乱数エンジンのインスタンスを作成してrdの結果で初期化する
	randomEngine_.seed (rd ());
	emitter_.count =3;
	emitter_.frequency = 0.3f;
	emitter_.frequencyTime = 0.0f;
}

MeshParticle::~MeshParticle () {
}

void MeshParticle::Initialize () {
	
}

void MeshParticle::Update (Matrix4x4* vp) {
	//Emitter更新
	EmitterUpdate ();

	vp_ = vp;

	//numInstanceのリセット
	numInstance_ = 0;
	uint32_t dstIndex = 0; //書き込み先インデックス
	for (particleIterator_ = particles_.begin (); particleIterator_ != particles_.end ();) {
		//生存可能時間を過ぎていたら更新処理をしない
		if (particleIterator_->lifeTime <= particleIterator_->currentTime) {
			particleIterator_ = particles_.erase (particleIterator_);	//生存時間をすぎたパーティクルはリストから削除
			continue;
		}

		if (dstIndex < kMaxParticleNum_) {
			//速度を反映させる
			particleIterator_->cube.transform.translate += particleIterator_->velocity * kDeltaTime;
			particleIterator_->cube.transform.rotate.x += randRotate_ (randomEngine_) * kDeltaTime;
			particleIterator_->cube.transform.rotate.y += randRotate_ (randomEngine_) * kDeltaTime;
			particleIterator_->cube.transform.rotate.z += randRotate_ (randomEngine_) * kDeltaTime;
			particleIterator_->currentTime += kDeltaTime;

			//徐々に透明度を下げて消えるように
			float alpha = 1.0f - (particleIterator_->currentTime / particleIterator_->lifeTime);
			for (int i = 0; i < 8; ++i) {
				particleIterator_->cube.color[i].w = alpha;
			}
		}

		//書き込み先インデックスをインクリメント
		++dstIndex;
		++particleIterator_;
	}
	numInstance_ = dstIndex;
}

void MeshParticle::Draw () {
	for (particleIterator_ = particles_.begin (); particleIterator_ != particles_.end (); ++particleIterator_) {
		Mesh::DrawCube (&particleIterator_->cube, *vp_);
	}
}

void MeshParticle::ImGui () {
	//BlendMode切り替え
	ImGui::Begin ("Particle");
	if (ImGui::Button ("spawn")) {
		particles_.splice (particles_.end (), Emit (emitter_, randomEngine_));
	}
	ImGui::Separator ();

	ImGui::Text ("Emitter");
	ImGui::DragFloat3 ("scale", &emitter_.transform.scale.x, 0.01f, 0.0f, 100.f);
	ImGui::DragFloat3 ("rotate", &emitter_.transform.rotate.x, 0.01f, -100.0f, 100.f);
	ImGui::DragFloat3 ("translate", &emitter_.transform.translate.x, 0.01f, -100.0f, 100.f);

	ImGui::End ();
}

MeshParticleData MeshParticle::MakeNewParticle (std::mt19937 randomEngine, const Emitter& emitter_) {
	//乱数エンジンのインスタンスを作成してrdの結果で初期化する
	randomEngine.seed (rd ());

	MeshParticleData data;

	//使う分布を初期化する
	pos_x = std::uniform_real_distribution<float> (-emitter_.transform.scale.x, emitter_.transform.scale.x);
	pos_y = std::uniform_real_distribution<float> (-emitter_.transform.scale.y, emitter_.transform.scale.y);
	pos_z = std::uniform_real_distribution<float> (-emitter_.transform.scale.z, emitter_.transform.scale.z);
	randRotate_ = std::uniform_real_distribution<float> (0.3f, 0.9f);
	randSize_ = std::uniform_real_distribution<float> (0.01f, 0.1f);
	randVelocity_ = std::uniform_real_distribution<float> (-5.0f, 5.0f);
	randColor_ = std::uniform_real_distribution<float> (0.0f, 1.0f);
	randTime_ = std::uniform_real_distribution<float> (10.0f, 20.0f);

	//パーティクル情報の初期化
	data.cube.transform.scale = { randSize_(randomEngine), randSize_ (randomEngine), randSize_ (randomEngine) };
	data.cube.transform.rotate = { 0.0f, 0.0f, 0.0f };
	data.cube.transform.translate = { pos_x (randomEngine), pos_y (randomEngine), pos_z (randomEngine) };
	data.velocity = { randVelocity_ (randomEngine), randVelocity_ (randomEngine), randVelocity_ (randomEngine) };
	for (uint32_t i = 0; i < 8; ++i) {
		data.cube.color[i] = {randColor_ (randomEngine), randColor_ (randomEngine), randColor_ (randomEngine), 1.0f};
	}
	data.lifeTime = randTime_ (randomEngine);
	data.currentTime = 0.0f;

	//emitterを加味してtranslateを再計算
	data.cube.transform.translate += emitter_.transform.translate;

	return data;
}

std::list<MeshParticleData> MeshParticle::Emit (const Emitter& emitter, std::mt19937& randomEngine) {
	std::list<MeshParticleData> particles;
	for (uint32_t count = 0; count < emitter.count; ++count) {
		particles.push_back (MakeNewParticle (randomEngine, emitter_));
	}
	return particles;
}

void MeshParticle::EmitterUpdate () {
	emitter_.frequencyTime += kDeltaTime;	//発生時刻を進める
	if (emitter_.frequency <= emitter_.frequencyTime) {		//頻度より大きいなら
		particles_.splice (particles_.end (), Emit (emitter_, randomEngine_));	//particle発生
		emitter_.frequencyTime -= emitter_.frequency;	//進めた時間を戻す
	}
}