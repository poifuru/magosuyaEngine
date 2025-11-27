#pragma once
#include "struct.h"

struct LineVertexData {
	Vector3 position;
	Vector4 color;
};

struct LineData {
	//頂点バッファ
	ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vbView{};
};

struct LineForGPU {
	Matrix4x4 World;
	Matrix4x4 WVP;
};