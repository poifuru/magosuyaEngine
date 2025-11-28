#pragma once
#include <stdint.h>

//メッシュごとに使用する頂点数
namespace VertexNum {
	inline const uint32_t Line = 2;
	inline const uint32_t Cube = 8;
}

//メッシュごとに出すことのできる最大数
namespace MaxMeshNum {
	inline const uint32_t Line = 10000;
	inline const uint32_t Cube = 10000;
}