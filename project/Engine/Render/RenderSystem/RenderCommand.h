#pragma once
#include "RenderingModel.h"

// RenderSystemが必要とする描画用情報の集合体
struct RenderCommand {
	// PSO
	ID3D12PipelineState* pso = nullptr;
	ID3D12PipelineState* shadowPso = nullptr;
	MyEngine::Rendering::ShadingModel shadingModel = MyEngine::Rendering::ShadingModel::Standard;
	MyEngine::Rendering::InputLayoutType inputLayout = MyEngine::Rendering::InputLayoutType::Standard3D;
	MyEngine::Rendering::BlendModeType blendMode = MyEngine::Rendering::BlendModeType::Opaque;
	bool depthEnable = true;
	bool depthWrite = true;
	bool doubleSided = false;


	// メッシュ情報
	D3D12_VERTEX_BUFFER_VIEW vbView{};
	D3D12_INDEX_BUFFER_VIEW ibv{};
	UINT vertexCount = 0;
	UINT indexCount = 0;

	// リソースやバッファのインデックス
	uint32_t materialIndex = 0;   // 構造化バッファ（マテリアル配列）の何番目か
	uint32_t textureIndex = 0;    // ディスクリプタヒープの何番目にテクスチャがあるか

	// オブジェクト個別のトランスフォームバッファのGPU仮想アドレス
	D3D12_GPU_VIRTUAL_ADDRESS transformGPUAddress = 0;

	// オブジェクト固有のカスタム定数バッファのアドレス
	D3D12_GPU_VIRTUAL_ADDRESS customBufferGPUAddress = 0;

	uint8_t layer = 0;            // 不透明：0、半透明：1 などのソート用

	// デバッグ用の名前を追加
	const char* debugName = "Unknown Object";
};