#pragma once
#include "struct.h"

struct CubeData {
	Vector3 center;
	float size;
	Vector3 position[8];
	Vector4 color[8];
};

//頂点バッファー確保用
struct CubeVertexPositionColor {
	Vector3 position;
	Vector4 color;
};

struct CubeVertexData {
	ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vbView{};

	ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW ibView{};
};

struct CubeForGPU {
	Matrix4x4 World;
	Matrix4x4 WVP;
};