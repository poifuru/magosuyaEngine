#pragma once
#include <Windows.h>
#include <Wrl.h>
using namespace Microsoft::WRL;
#include <d3d12.h>
#include <vector>
#include <array>
#include <memory>
#include <random>
#include "struct.h"
#include "PSOManager.h"

class DxCommon;

class Particle {
public:
	Particle (DxCommon* dxCommon);
	~Particle ();

	void Initialize ();
	void Update (Matrix4x4* cameraMatrix, Matrix4x4* vp);
	void Draw ();
	void ImGui ();

	uint32_t GetParticleNum () { return kMaxParticleNum_; };
	void SetTexHandle (D3D12_GPU_DESCRIPTOR_HANDLE handle) { handle_ = handle; }

private:	//ヘルパー関数
	ParticleData MakeNewParticle (std::mt19937 randomEngine);

private:
	//PSOの設定
	PSODescriptor desc_ = {};
	ID3D12RootSignature* rootsignature_ = nullptr;
	ID3D12PipelineState* pipelineState_ = nullptr;

	//モデルデータ
	std::unique_ptr<ModelData> data_ = nullptr;
	//何個のパーティクルを出すのか
	const uint32_t kMaxParticleNum_ = 10;
	uint32_t numInstance_ = 0; //描画すべきインスタンス数

	//GPUリソース
	ComPtr<ID3D12Resource> instancingBuffer_;
	ComPtr<ID3D12Resource> materialBuffer_;

	//マッピング用のCPUデータ
	VertexData* vertexData_ = nullptr;
	uint32_t* indexData_ = nullptr;
	ParticleForGPU* instancingData_ = nullptr;
	Material* materialData_ = nullptr;

	//テクスチャハンドル
	D3D12_CPU_DESCRIPTOR_HANDLE particleSrvHandleCPU = {};
	D3D12_GPU_DESCRIPTOR_HANDLE particleSrvHandleGPU = {};
	D3D12_GPU_DESCRIPTOR_HANDLE handle_ = {};

	//位置データ
	ParticleData particles_[10];
	Transform uvTransform_[10];

	//速度をランダムに割り当てるための乱数生成器
	//生成エンジンの型を作る
	using RNG_Engine = std::mt19937;
	//作った型でエンジンの宣言
	RNG_Engine randomEngine_;
	//実行ごとに異なる値を取得する
	std::random_device rd;
	//分布
	std::uniform_real_distribution<float> rand_;		//速度
	std::uniform_real_distribution<float> randColor_;	//色
	std::uniform_real_distribution<float> randTime_;	//パーティクルの生存可能時間

	//ビルボードの変数
	bool useBillBoard = false;
	Matrix4x4 billBoardMatrix_ = {};

	//ImGui用の変数
	int currentBlendMode_ = static_cast<int>(BlendModeType::Opaque); // 現在のブレンドモードのインデックス
	const char* blendModeNames_[6] = { "Opaque", "Alpha", "Additive", "Subtract", "Multiply", "Screen" };
	const int kBlendModeCount_ = 6;

	//ポインタを借りる
	DxCommon* dxCommon_ = nullptr;
	ID3D12Device* device_ = nullptr;
	ID3D12GraphicsCommandList* commandList_ = nullptr;
};

