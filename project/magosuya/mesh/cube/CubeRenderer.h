#pragma once
#include <Windows.h>
#include <Wrl.h>
using namespace Microsoft::WRL;
#include <d3d12.h>
#include <memory>
#include "struct.h"
#include "CubeRenderData.h"
#include "PSOManager.h"

class DxCommon;

class CubeRenderer {
public:
	static CubeRenderer* GetInstance () {
		//初めて呼び出されたときに一回だけ初期化
		static CubeRenderer instance;
		return &instance;
	}
	~CubeRenderer ();

	void Initialize (DxCommon* dxCommon);
	void UpdateVertexData (const CubeData* data);
	void TransferData (const CubeForGPU& data);
	void Draw ();

private:	//内部関数
	void ResetCurrentCubeCount () { currentCubeCount_ = 0; }

private:
	//コンストラクタを禁止
	CubeRenderer () = default;
	// コピーコンストラクタと代入演算子を禁止
	CubeRenderer (const CubeRenderer&) = delete;
	CubeRenderer& operator=(const CubeRenderer&) = delete;
	CubeRenderer (CubeRenderer&&) = delete;
	CubeRenderer& operator=(CubeRenderer&&) = delete;

private:
	//使うディスクリプターの場所
	const uint32_t descriptorIndex_ = 102;

	//現在描画要求のあるCubeの数
	uint32_t currentCubeCount_ = 0;

	//PSOの設定
	PSODescriptor desc_ = {};

	//GPUリソースとマッピング用データ
	std::unique_ptr<CubeVertexData> cubeBuffer_ = nullptr;
	CubeVertexPositionColor* vertexData_ = nullptr;

	uint32_t* indexData_ = nullptr;

	ComPtr<ID3D12Resource> instancingBuffer_ = nullptr;
	CubeForGPU* instancingData_ = nullptr;

	//ディスクリプタハンドル
	D3D12_CPU_DESCRIPTOR_HANDLE cubeSrvHandleCPU_ = {};
	D3D12_GPU_DESCRIPTOR_HANDLE cubeSrvHandleGPU_ = {};
	D3D12_CPU_DESCRIPTOR_HANDLE vertexSrvHandleCPU_ = {};
	D3D12_GPU_DESCRIPTOR_HANDLE vertexSrvHandleGPU_ = {};

	//ポインタを借りる
	DxCommon* dxCommon_ = nullptr;
	ID3D12Device* device_ = nullptr;
	ID3D12GraphicsCommandList* commandList_ = nullptr;
};