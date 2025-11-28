#pragma once
#include <Windows.h>
#include <Wrl.h>
using namespace Microsoft::WRL;
#include <d3d12.h>
#include <memory>
#include "struct.h"
#include "LineRenderData.h"
#include "PSOManager.h"

class DxCommon;

class LineRenderer {
public:
	static LineRenderer* GetInstance () {
		//初めて呼び出されたときに一回だけ初期化
		static LineRenderer instance;
		return &instance;
	}
	~LineRenderer ();

	void Initialize (DxCommon* dxCommon);
	void UpdateVertexData (const LineVertexData* data);
	void TransferData (const LineForGPU& data);
	void Draw ();

private:	//内部関数
	void ResetCurrentLineCount () { currentLineCount_ = 0; }

private:
	//コンストラクタを禁止
	LineRenderer () = default;
	// コピーコンストラクタと代入演算子を禁止
	LineRenderer (const LineRenderer&) = delete;
	LineRenderer& operator=(const LineRenderer&) = delete;
	LineRenderer (LineRenderer&&) = delete;
	LineRenderer& operator=(LineRenderer&&) = delete;

private:
	//使うディスクリプターの場所
	const uint32_t descriptorIndex_ = 100;

	//現在描画要求のあるLineの数
	uint32_t currentLineCount_ = 0;

	//PSOの設定
	PSODescriptor desc_ = {};

	//GPUリソースとマッピング用データ
	std::unique_ptr<LineData> lineBuffer_ = nullptr;
	LineVertexData* vertexData_ = nullptr;

	ComPtr<ID3D12Resource> instancingBuffer_ = nullptr;
	LineForGPU* instancingData_ = nullptr;

	//ディスクリプタハンドル
	D3D12_CPU_DESCRIPTOR_HANDLE lineSrvHandleCPU_ = {};
	D3D12_GPU_DESCRIPTOR_HANDLE lineSrvHandleGPU_ = {};
	D3D12_CPU_DESCRIPTOR_HANDLE vertexSrvHandleCPU_ = {};
	D3D12_GPU_DESCRIPTOR_HANDLE vertexSrvHandleGPU_ = {};

	//ポインタを借りる
	DxCommon* dxCommon_ = nullptr;
	ID3D12Device* device_ = nullptr;
	ID3D12GraphicsCommandList* commandList_ = nullptr;
};