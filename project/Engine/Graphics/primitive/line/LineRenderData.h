#pragma once
#include "MathTypes.h"

// 線の頂点（ワールド座標のまま GPU に渡す）
// InputLayoutType::LineMesh（POSITION float3 + COLOR float4、APPEND_ALIGNED）と同じ並びにするため、パディングは入れない
struct LineVertex {
	Vector3 position; // ワールド座標
	Vector4 color;    // RGBA
};
static_assert(sizeof(LineVertex) == 28, "LineVertex と InputLayoutType::LineMesh がずれている");
