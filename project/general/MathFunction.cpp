#include "MathFunction.h"
//#include <Novice.h>
#include <cmath>
#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>

Vector3 Add(const Vector3& v1, const Vector3& v2) {
	return {
		v1.x + v2.x,
		v1.y + v2.y,
		v1.z + v2.z
	};
}

Vector3 Subtract(const Vector3& v1, const Vector3& v2) {
	return {
		v1.x - v2.x,
		v1.y - v2.y,
		v1.z - v2.z
	};
}

Vector3 Multiply(float scalar, const Vector3& v) {
	return {
		scalar * v.x,
		scalar * v.y,
		scalar * v.z
	};
}

float Dot(const Vector3& v1, const Vector3& v2) {
	return {
		(v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z)
	};
}

float Length(const Vector3& v) {
	return {
		sqrtf(powf(v.x, 2) + powf(v.y, 2) + powf(v.z, 2))
	};
}

Vector3 Normalize(const Vector3& v) {
	float nor = sqrtf(powf(v.x, 2) + powf(v.y, 2) + powf(v.z, 2));

	return {
		v.x / nor,
		v.y / nor,
		v.z / nor
	};
}

Vector3 Cross(const Vector3& v1, const Vector3& v2) {
	Vector3 result = {
		v1.y * v2.z - v1.z * v2.y,
		v1.z * v1.x - v1.x * v2.z,
		v1.x * v2.y - v1.y * v2.x
	};

	return result;
}

Vector4 Multiply(const Matrix4x4& mat, const Vector4& vec)
{
	Vector4 result;

	result.x = mat.m[0][0] * vec.x + mat.m[0][1] * vec.y + mat.m[0][2] * vec.z + mat.m[0][3] * vec.w;
	result.y = mat.m[1][0] * vec.x + mat.m[1][1] * vec.y + mat.m[1][2] * vec.z + mat.m[1][3] * vec.w;
	result.z = mat.m[2][0] * vec.x + mat.m[2][1] * vec.y + mat.m[2][2] * vec.z + mat.m[2][3] * vec.w;
	result.w = mat.m[3][0] * vec.x + mat.m[3][1] * vec.y + mat.m[3][2] * vec.z + mat.m[3][3] * vec.w;

	return result;
}

Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2)
{
	Matrix4x4 result;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			result.m[i][j] = m1.m[i][j] + m2.m[i][j];
		}
	}
	return result;
}

Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2)
{
	Matrix4x4 result;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			result.m[i][j] = m1.m[i][j] - m2.m[i][j];
		}
	}
	return result;
}

Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2)
{
	Matrix4x4 result;

	for (int i = 0; i < 4; ++i) {         // 行
		for (int j = 0; j < 4; ++j) {     // 列
			result.m[i][j] = 0.0f;
			for (int k = 0; k < 4; ++k) { // 積の途中経過
				result.m[i][j] += m1.m[i][k] * m2.m[k][j];
			}
		}
	}

	return result;
}

//演算子オーバーロード
Vector3 operator+(const Vector3& v1, const Vector3& v2) { return Add(v1, v2); }
Vector3 operator-(const Vector3& v1, const Vector3& v2) { return Subtract(v1, v2); }
Vector3 operator*(float s, const Vector3& v) { return Multiply(s, v); }
Vector3 operator*(const Vector3& v, float s) { return s * v; }
Vector3 operator/(const Vector3& v, float s) { return Multiply(1.0f / s, v); }
Matrix4x4 operator+(const Matrix4x4& m1, const Matrix4x4& m2) { return Add(m1, m2); }
Matrix4x4 operator-(const Matrix4x4& m1, const Matrix4x4& m2) { return Subtract(m1, m2); }
Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4& m2) { return Multiply(m1, m2); }
/*単項演算子*/
Vector3 operator+(const Vector3& v) { return { -v.x, -v.y, -v.z }; }
Vector3 operator-(const Vector3& v) { return v; }

//逆行列補助用の関数
float Minor(const Matrix4x4& mat, int row, int col) {
	float sub[3][3];
	int r = 0, c = 0;

	for (int i = 0; i < 4; ++i) {
		if (i == row) continue;
		c = 0;
		for (int j = 0; j < 4; ++j) {
			if (j == col) continue;
			sub[r][c] = mat.m[i][j];
			++c;
		}
		++r;
	}

	// 3x3行列の行列式を計算
	return
		sub[0][0] * (sub[1][1] * sub[2][2] - sub[1][2] * sub[2][1]) -
		sub[0][1] * (sub[1][0] * sub[2][2] - sub[1][2] * sub[2][0]) +
		sub[0][2] * (sub[1][0] * sub[2][1] - sub[1][1] * sub[2][0]);
}

// 4x4行列の行列式
float Determinant(const Matrix4x4& mat) {
	float det = 0.0f;
	for (int col = 0; col < 4; ++col) {
		float sign = (col % 2 == 0) ? 1.0f : -1.0f;
		det += sign * mat.m[0][col] * Minor(mat, 0, col);
	}
	return det;
}

Matrix4x4 Inverse(const Matrix4x4& m)
{
	Matrix4x4 result;
	float det = Determinant(m);

	// ゼロ除算防止（特異行列）
	if (std::fabs(det) < 1e-6f) {
		// ゼロ行列返す
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				result.m[i][j] = 0.0f;
		return result;
	}

	// 余因子行列（cofactor matrix）
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			float sign = ((i + j) % 2 == 0) ? 1.0f : -1.0f;
			result.m[j][i] = sign * Minor(m, i, j); // 転置して代入（adjugate）
		}
	}

	// 行列式で割る（スカラー倍）
	float invDet = 1.0f / det;
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			result.m[i][j] *= invDet;

	return result;
}

Matrix4x4 Transpose(const Matrix4x4& m)
{
	Matrix4x4 result;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			result.m[i][j] = m.m[j][i];
		}
	}
	return result;
}

Matrix4x4 MakeIdentity4x4()
{
	Matrix4x4 result = { 0.0f };

	for (int i = 0; i < 4; ++i) {
		result.m[i][i] = 1.0f; // 対角成分だけ 1 にする
	}

	return result;
}

Matrix4x4 MakeTranslateMatrix(const Vector3& translate)
{
	Matrix4x4 result = { 0.0f };

	result.m[0][0] = 1.0f;
	result.m[1][1] = 1.0f;
	result.m[2][2] = 1.0f;
	result.m[3][3] = 1.0f;

	result.m[3][0] = translate.x;
	result.m[3][1] = translate.y;
	result.m[3][2] = translate.z;

	return result;
}

Matrix4x4 MakeScaleMatrix(const Vector3& scale)
{
	Matrix4x4 result = { 0.0f };

	result.m[0][0] = scale.x;
	result.m[1][1] = scale.y;
	result.m[2][2] = scale.z;
	result.m[3][3] = 1.0f;

	return result;
}

Vector3 ChangeTransform(const Vector3& vector, const Matrix4x4& matrix)
{
	Vector3 result;

	float x = vector.x;
	float y = vector.y;
	float z = vector.z;
	float w = 1.0f;

	//変換する
	float transformX = matrix.m[0][0] * x + matrix.m[1][0] * y + matrix.m[2][0] * z + matrix.m[3][0] * w;
	float transformY = matrix.m[0][1] * x + matrix.m[1][1] * y + matrix.m[2][1] * z + matrix.m[3][1] * w;
	float transformZ = matrix.m[0][2] * x + matrix.m[1][2] * y + matrix.m[2][2] * z + matrix.m[3][2] * w;
	float transformW = matrix.m[0][3] * x + matrix.m[1][3] * y + matrix.m[2][3] * z + matrix.m[3][3] * w;

	//同次座標のwで割って正規化する。
	if (transformW != 0.0f) {
		result.x = transformX / transformW;
		result.y = transformY / transformW;
		result.z = transformZ / transformW;
	}
	else {//transformWが0ならそのまま使う
		result.x = transformX;
		result.y = transformY;
		result.z = transformZ;
	}

	return result;
}

Matrix4x4 MakeRotateXMatrix(float radian)
{
	Matrix4x4 result = MakeIdentity4x4();

	result.m[1][1] = cosf(radian);
	result.m[1][2] = sinf(radian);
	result.m[2][1] = -sinf(radian);
	result.m[2][2] = cosf(radian);

	return result;
}

Matrix4x4 MakeRotateYMatrix(float radian)
{
	Matrix4x4 result = MakeIdentity4x4();

	result.m[0][0] = cosf(radian);
	result.m[0][2] = -sinf(radian);
	result.m[2][0] = sinf(radian);
	result.m[2][2] = cosf(radian);

	return result;
}

Matrix4x4 MakeRotateZMatrix(float radian)
{
	Matrix4x4 result = MakeIdentity4x4();

	result.m[0][0] = cosf(radian);
	result.m[0][1] = sinf(radian);
	result.m[1][0] = -sinf(radian);
	result.m[1][1] = cosf(radian);

	return result;
}

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate)
{
	Matrix4x4 matrix = MakeIdentity4x4();

	Matrix4x4 scaleMatrix = Multiply(matrix, MakeScaleMatrix(scale));
	Matrix4x4 rotateXMatrix = Multiply(scaleMatrix, MakeRotateXMatrix(rotate.x));
	Matrix4x4 rotateXYMatrix = Multiply(rotateXMatrix, MakeRotateYMatrix(rotate.y));
	Matrix4x4 rotateXYZMatrix = Multiply(rotateXYMatrix, MakeRotateZMatrix(rotate.z));
	Matrix4x4 translateMatrix = Multiply(rotateXYZMatrix, MakeTranslateMatrix(translate));

	return translateMatrix;
}

Matrix4x4 MakePerspectiveFOVMatrix(float fovY, float aspectRatio, float nearClip, float farClip)
{
	Matrix4x4 result = { 0.0f };

	result.m[0][0] = (1 / aspectRatio) * (1 / tanf(fovY / 2));
	result.m[1][1] = 1 / tanf(fovY / 2);
	result.m[2][2] = farClip / (farClip - nearClip);
	result.m[2][3] = 1.0f;
	result.m[3][2] = (-nearClip * farClip) / (farClip - nearClip);

	return result;
}

Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip)
{
	Matrix4x4 result = MakeIdentity4x4();

	result.m[0][0] = 2 / (right - left);
	result.m[1][1] = 2 / (top - bottom);
	result.m[2][2] = 1 / (farClip - nearClip);
	result.m[3][0] = (left + right) / (left - right);
	result.m[3][1] = (top + bottom) / (bottom - top);
	result.m[3][2] = nearClip / (nearClip - farClip);

	return result;
}

Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth)
{
	Matrix4x4 result = MakeIdentity4x4();

	result.m[0][0] = (width / 2);
	result.m[1][1] = -(height / 2);
	result.m[2][2] = maxDepth - minDepth;
	result.m[3][0] = left + (width / 2);
	result.m[3][1] = top + (height / 2);
	result.m[3][2] = minDepth;

	return result;
}

//void VectorScreenPrintf(int x, int y, const Vector3& vector, const char* label) {
//	Novice::ScreenPrintf(x, y, "%0.2f", vector.x);
//	Novice::ScreenPrintf(x + kColumnWidth, y, "%0.2f", vector.y);
//	Novice::ScreenPrintf(x + kColumnWidth * 2, y, "%0.2f", vector.z);
//	Novice::ScreenPrintf(x + kColumnWidth * 3, y, "%s", label);
//}

//void MatrixScreenPrint(int x, int y, const Matrix4x4& m, const char* str) {
//	Novice::ScreenPrintf(x, y, "%s", str);
//	for (int row = 0; row < 4; ++row) {
//		for (int column = 0; column < 4; ++column) {
//			Novice::ScreenPrintf(
//				x + column * kColumnWidth, y + 20 + row * kRowHeight, "%6.02f", m.m[row][column]
//			);
//		}
//	}
//}

//void DrawGrid(const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix) {
//	const float kGridHalfWidth = 2.0f;											//Gridの半分の幅
//	const uint32_t kSubdivision = 10;											//分割数
//	const float kGridEvery = (kGridHalfWidth * 2.0f) / float(kSubdivision);		//1つ分の長さ
//
//	Vector3 start;
//	Vector3 end;
//	Vector3 ndcVertexStart;
//	Vector3 ndcVertexEnd;
//	Vector3 screenStart;
//	Vector3 screenEnd;
//
//	//奥から手前への線を順々に引いていく
//	for (uint32_t xIndex = 0; xIndex <= kSubdivision; ++xIndex) {
//		float x = -kGridHalfWidth + kGridEvery * xIndex;
//
//		start = Vector3(x, 0.0f, kGridHalfWidth);
//		end = Vector3(x, 0.0f, -kGridHalfWidth);
//
//		//NDC変換
//		ndcVertexStart = Transform(start, viewProjectionMatrix);
//		ndcVertexEnd = Transform(end, viewProjectionMatrix);
//
//		screenStart = Transform(ndcVertexStart, viewportMatrix);
//		screenEnd = Transform(ndcVertexEnd, viewportMatrix);
//
//		// 線を描画（薄い灰色、真ん中だけ黒）
//		if (xIndex == 5) {
//			Novice::DrawLine(
//				static_cast<int>(screenStart.x), static_cast<int>(screenStart.y),
//				static_cast<int>(screenEnd.x), static_cast<int>(screenEnd.y), 0x000000FF
//			);
//		}
//		else {
//			Novice::DrawLine(
//				static_cast<int>(screenStart.x), static_cast<int>(screenStart.y),
//				static_cast<int>(screenEnd.x), static_cast<int>(screenEnd.y), 0xAAAAAAFF
//			);
//		}
//	}
//
//	//左から右への線を順々に引いていく
//	for (uint32_t zIndex = 0; zIndex <= kSubdivision; ++zIndex) {
//		float z = -kGridHalfWidth + kGridEvery * zIndex;
//
//		start = Vector3(-kGridHalfWidth, 0.0f, z);
//		end = Vector3(kGridHalfWidth, 0.0f, z);
//
//		//NDC変換
//		ndcVertexStart = Transform(start, viewProjectionMatrix);
//		ndcVertexEnd = Transform(end, viewProjectionMatrix);
//
//		screenStart = Transform(ndcVertexStart, viewportMatrix);
//		screenEnd = Transform(ndcVertexEnd, viewportMatrix);
//
//		// 線を描画（薄い灰色、真ん中だけ黒）
//		if (zIndex == 5) {
//			Novice::DrawLine(
//				static_cast<int>(screenStart.x), static_cast<int>(screenStart.y),
//				static_cast<int>(screenEnd.x), static_cast<int>(screenEnd.y), 0x000000FF
//			);
//		}
//		else {
//			Novice::DrawLine(
//				static_cast<int>(screenStart.x), static_cast<int>(screenStart.y),
//				static_cast<int>(screenEnd.x), static_cast<int>(screenEnd.y), 0xAAAAAAFF
//			);
//		}
//	}
//}

//void DrawSphere(const Vector3& center, const float& radius, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color) {
//	const uint32_t kLatSubdivision = 10;		//分割数
//	const uint32_t kLonSubdivision = 20;
//	const float kLatEvery = static_cast<float>(M_PI) / kLatSubdivision;				//経度1つ分の角度
//	const float kLonEvery = static_cast<float>(M_PI * 2.0f) / kLonSubdivision;		//緯度1つ分の角度
//
//	//緯度(横)の方向に分割 -π/2 ~ π/2[
//	for (uint32_t latIndex = 0; latIndex < kLatSubdivision; ++latIndex) {
//		float lat = float(-M_PI) / 2.0f + kLatEvery * latIndex;	//現在の緯度 
//		//経度(縦)の方向に分割 0~2π
//		for (uint32_t lonIndex = 0; lonIndex < kLonSubdivision; ++lonIndex) {
//			float lon = lonIndex * kLonEvery;	//現在の経度
//			//world座標系でのa,b,cを求める
//			Vector3 a, b, c;
//			Vector3 ndcVertexA, ndcVertexB, ndcVertexC;
//			Vector3 ScreenA, ScreenB, ScreenC;
//			a = Vector3(
//				cosf(lon) * cosf(lat) * radius + center.x,
//				sinf(lon) * radius + center.y,
//				cosf(lon) * sinf(lat) * radius + center.z
//			);
//
//			b = Vector3(
//				cosf(kLonEvery + lon) * cosf(lat) * radius + center.x,
//				sinf(kLonEvery + lon) * radius + center.y,
//				cosf(kLonEvery + lon) * sinf(lat) * radius + center.z
//			);
//
//			c = Vector3(
//				cosf(lon) * cosf(kLatEvery + lat) * radius + center.x,
//				sinf(lon) * radius + center.y,
//				cosf(lon) * sinf(kLatEvery + lat) * radius + center.z
//			);
//
//			//a,b,cをScreen座標まで変換
//			//NDC変換
//			ndcVertexA = Transform(a, viewProjectionMatrix);
//			ndcVertexB = Transform(b, viewProjectionMatrix);
//			ndcVertexC = Transform(c, viewProjectionMatrix);
//
//			ScreenA = Transform(ndcVertexA, viewportMatrix);
//			ScreenB = Transform(ndcVertexB, viewportMatrix);
//			ScreenC = Transform(ndcVertexC, viewportMatrix);
//
//			//ab,bcで線を引く
//			Novice::DrawLine(
//				static_cast<int>(ScreenA.x), static_cast<int>(ScreenA.y),
//				static_cast<int>(ScreenB.x), static_cast<int>(ScreenB.y),
//				color
//			);
//			Novice::DrawLine(
//				static_cast<int>(ScreenA.x), static_cast<int>(ScreenA.y),
//				static_cast<int>(ScreenC.x), static_cast<int>(ScreenC.y),
//				color
//			);
//		}
//	}
//}

Vector3 Project(const Vector3& v1, const Vector3& v2) {
	Vector3 result{};

	float dot = Dot(v1, v2);
	float length = Length(v2);

	if (length != 0.0f) {
		result.x = (dot / powf(length, 2)) * v2.x;
		result.y = (dot / powf(length, 2)) * v2.y;
		result.z = (dot / powf(length, 2)) * v2.z;
	};

	return result;
}

Vector3 ClosestPoint(const Vector3& point, const Segment& segment) {
	Vector3 ab = Subtract(segment.diff, segment.origin);
	Vector3 ap = Subtract(point, segment.origin);

	float t = Dot(ap, ab) / (powf(ab.x, 2) + powf(ab.y, 2) + powf(ab.z, 2));

	if (t < 0) {
		return segment.origin;
	}
	if (t > 1) {
		return segment.diff;
	}
	else {
		Vector3 projection = Add(segment.origin, Multiply(t, ab));
		return projection;
	}
}

bool IsCollision(const Sphere& s1, const Sphere& s2) {
	float dis = Length(Subtract(s1.center, s2.center));
	float radiusDis = s1.radius + s2.radius;

	return dis <= radiusDis;
}

bool IsCollision(const Sphere& sphere, const Plane& plane) {
	float k = Dot(plane.normal, sphere.center) - plane.distance;
	return fabs(k) <= sphere.radius;
}

Vector3 Perpendicular(const Vector3& vector) {
	if (vector.x != 0.0f || vector.y != 0.0f) {
		return { -vector.y, vector.x, 0.0f };
	}
	return { 0.0f, -vector.z, vector.y };
}

//void DrawPlane(const Plane& plane, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color) {
//	Vector3 center = Multiply(plane.distance, plane.normal);	//1
//	Vector3 prependiculars[4];
//	prependiculars[0] = Normalize(Perpendicular(plane.normal));	//2
//	prependiculars[1] = { -prependiculars[0].x, -prependiculars[0].y, -prependiculars[0].z };	//3
//	prependiculars[2] = Cross(plane.normal, prependiculars[0]);	//4
//	prependiculars[3] = { -prependiculars[2].x, -prependiculars[2].y, -prependiculars[2].z };	//5
//	//6
//	Vector3 points[4];
//	for (uint32_t index = 0; index < 4; ++index) {
//		Vector3 extend = Multiply(2.0f, prependiculars[index]);
//		Vector3 point = Add(center, extend);
//		points[index] = Transform(Transform(point, viewProjectionMatrix), viewportMatrix);
//	}
//	//pointをそれぞれ結んでDrawLineで矩形を描画する。DrawTriangleを使って塗りつぶしてもよいが、DepthがないのでMT3では分かりづらい
//	Novice::DrawLine(static_cast<int>(points[0].x), static_cast<int>(points[0].y), static_cast<int>(points[2].x), static_cast<int>(points[2].y), color);
//	Novice::DrawLine(static_cast<int>(points[1].x), static_cast<int>(points[1].y), static_cast<int>(points[3].x), static_cast<int>(points[3].y), color);
//	Novice::DrawLine(static_cast<int>(points[0].x), static_cast<int>(points[0].y), static_cast<int>(points[3].x), static_cast<int>(points[3].y), color);
//	Novice::DrawLine(static_cast<int>(points[1].x), static_cast<int>(points[1].y), static_cast<int>(points[2].x), static_cast<int>(points[2].y), color);
//}

//bool IsCollision(const Line& line, const Plane& plane) {
//
//	return false;
//}
//
//bool IsCollision(const Ray& ray, const Plane& plane) {
//	return false;
//}
//
bool IsCollision(const Segment& segment, const Plane& plane) {
	float dot = Dot(plane.normal, segment.diff);

	if (dot == 0.0f) {
		return false;
	}

	//tを求める
	float t = (plane.distance - Dot(segment.origin, plane.normal)) / dot;

	//tの値と線の種類によって衝突しているか判定する
	return t > 0.0f && t < 1.0f;
}

bool IsCollision(const Triangle& triangle, const Segment& segment) {
	Vector3 v0 = triangle.vertices[0];
	Vector3 v1 = triangle.vertices[1];
	Vector3 v2 = triangle.vertices[2];

	Vector3 edge0 = Subtract(v1, v0);
	Vector3 edge1 = Subtract(v2, v1);
	Vector3 edge2 = Subtract(v0, v2);

	// 法線ベクトル
	Vector3 normal = Cross(edge0, Subtract(v2, v0));

	// セグメントと平面の交差チェック
	float dot = Dot(normal, segment.diff);
	if (dot == 0.0f) {
		return false;
	}

	float t = (Dot(normal, v0) - Dot(segment.origin, normal)) / dot;
	if (t < 0.0f || t > 1.0f) {
		return false;
	}

	// 交点を求める
	Vector3 P = Add(segment.origin, Multiply(t, segment.diff));

	// 三角形内にあるかチェック
	Vector3 c0 = Subtract(P, v0);
	Vector3 c1 = Subtract(P, v1);
	Vector3 c2 = Subtract(P, v2);

	if (Dot(Cross(edge0, c0), normal) >= 0.0f &&
		Dot(Cross(edge1, c1), normal) >= 0.0f &&
		Dot(Cross(edge2, c2), normal) >= 0.0f) {
		return true;
	}

	return false;
}

//void DrawTriangle(const Triangle& triangle, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color) {
//	Triangle screenTriangle;
//	for (int i = 0; i < 3; i++) {
//		screenTriangle.vertices[i] = Transform(Transform(triangle.vertices[i], viewProjectionMatrix), viewportMatrix);
//	}
//
//	Novice::DrawTriangle(
//		int(screenTriangle.vertices[0].x), int(screenTriangle.vertices[0].y),
//		int(screenTriangle.vertices[1].x), int(screenTriangle.vertices[1].y),
//		int(screenTriangle.vertices[2].x), int(screenTriangle.vertices[2].y),
//		color, kFillModeWireFrame
//	);
//}

bool IsCollision(const AABB& aabb1, const AABB& aabb2) {
	return
		(aabb1.min.x <= aabb1.max.x && aabb1.max.x >= aabb2.min.x) &&
		(aabb1.min.y <= aabb1.max.y && aabb1.max.y >= aabb2.min.y) &&
		(aabb1.min.z <= aabb1.max.z && aabb1.max.z >= aabb2.min.z);
}

//void DrawAABB(const AABB& aabb, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color) {
//	//引数を元に八つの頂点を作る
//	Vector3 vertex[8];
//	Vector3 screenVertex[8];
//
//	vertex[0] = { aabb.min.x, aabb.max.y, aabb.min.z };
//	vertex[1] = { aabb.max.x, aabb.max.y, aabb.min.z };
//	vertex[2] = { aabb.max.x, aabb.max.y, aabb.max.z };
//	vertex[3] = { aabb.min.x, aabb.max.y, aabb.max.z };
//	vertex[4] = { aabb.min.x, aabb.min.y, aabb.min.z };
//	vertex[5] = { aabb.max.x, aabb.min.y, aabb.min.z };
//	vertex[6] = { aabb.max.x, aabb.min.y, aabb.max.z };
//	vertex[7] = { aabb.min.x, aabb.min.y, aabb.max.z };
//
//	for (int i = 0; i < 8; i++) {
//		screenVertex[i] = Transform(Transform(vertex[i], viewProjectionMatrix), viewportMatrix);
//	}
//
//	Novice::DrawLine(int(screenVertex[0].x), int(screenVertex[0].y), int(screenVertex[1].x), int(screenVertex[1].y), color);
//	Novice::DrawLine(int(screenVertex[1].x), int(screenVertex[1].y), int(screenVertex[2].x), int(screenVertex[2].y), color);
//	Novice::DrawLine(int(screenVertex[2].x), int(screenVertex[2].y), int(screenVertex[3].x), int(screenVertex[3].y), color);
//	Novice::DrawLine(int(screenVertex[3].x), int(screenVertex[3].y), int(screenVertex[0].x), int(screenVertex[0].y), color);
//	Novice::DrawLine(int(screenVertex[4].x), int(screenVertex[4].y), int(screenVertex[5].x), int(screenVertex[5].y), color);
//	Novice::DrawLine(int(screenVertex[5].x), int(screenVertex[5].y), int(screenVertex[6].x), int(screenVertex[6].y), color);
//	Novice::DrawLine(int(screenVertex[6].x), int(screenVertex[6].y), int(screenVertex[7].x), int(screenVertex[7].y), color);
//	Novice::DrawLine(int(screenVertex[7].x), int(screenVertex[7].y), int(screenVertex[4].x), int(screenVertex[4].y), color);
//	Novice::DrawLine(int(screenVertex[0].x), int(screenVertex[0].y), int(screenVertex[4].x), int(screenVertex[4].y), color);
//	Novice::DrawLine(int(screenVertex[1].x), int(screenVertex[1].y), int(screenVertex[5].x), int(screenVertex[5].y), color);
//	Novice::DrawLine(int(screenVertex[2].x), int(screenVertex[2].y), int(screenVertex[6].x), int(screenVertex[6].y), color);
//	Novice::DrawLine(int(screenVertex[3].x), int(screenVertex[3].y), int(screenVertex[7].x), int(screenVertex[7].y), color);
//}

bool IsCollision(const AABB& aabb, const Sphere& sphere) {
	//最近接点を求める
	Vector3 closestPoint{
		std::clamp(sphere.center.x, aabb.min.x, aabb.max.x),
		std::clamp(sphere.center.y, aabb.min.y, aabb.max.y),
		std::clamp(sphere.center.z, aabb.min.z, aabb.max.z)
	};
	//最近接点と球のの中心との距離を求める//
	//球の中心と最近接点との差分ベクトル
	Vector3 vector = Subtract(sphere.center, closestPoint);
	float distance = Length(vector);

	return distance <= sphere.radius;
}

bool IsCollision(const AABB& aabb, const Segment& segment) {
	Vector3 dir = Subtract(segment.diff, segment.origin);
	float tmin = 0.0f;
	float tmax = 1.0f;

	// 各軸ごとに判定
	for (int i = 0; i < 3; ++i) {
		float origin = (&segment.origin.x)[i];
		float direction = (&dir.x)[i];
		float min = (&aabb.min.x)[i];
		float max = (&aabb.max.x)[i];

		if (fabs(direction) < 1e-6f) {
			// 線分がこの軸に平行 → AABBの内側にあるか
			if (origin < min || origin > max) {
				return false;
			}
		}
		else {
			float invD = 1.0f / direction;
			float t1 = (min - origin) * invD;
			float t2 = (max - origin) * invD;

			if (t1 > t2) std::swap(t1, t2);

			tmin = max(tmin, t1);
			tmax = min(tmax, t2);

			if (tmin > tmax) {
				return false;
			}
		}
	}

	return true;
}

//void objectRotation(Vector3& rotate) {
//	static int prevMouseX = 0;
//	static int prevMouseY = 0;
//	static bool isDragging = false;
//	static bool wasMousePressed = false;
//
//	int currentMouseX = 0;
//	int currentMouseY = 0;
//
//	bool isMousePressed = Novice::IsPressMouse(0);
//	const float rotationSpeed = 0.01f; // ← ここで動きの速さを調整
//
//	if (!wasMousePressed && isMousePressed) {
//		isDragging = true;
//		Novice::GetMousePosition(&prevMouseX, &prevMouseY);
//	}
//
//	if (isMousePressed && isDragging) {
//		Novice::GetMousePosition(&currentMouseX, &currentMouseY);
//
//		int dx = currentMouseX - prevMouseX;
//		int dy = currentMouseY - prevMouseY;
//
//		// スケーリングして回転に加える
//		rotate.x -= static_cast<float>(dy) * rotationSpeed;
//		rotate.y -= static_cast<float>(dx) * rotationSpeed;
//
//		prevMouseX = currentMouseX;
//		prevMouseY = currentMouseY;
//	}
//
//	if (wasMousePressed && !isMousePressed) {
//		isDragging = false;
//	}
//
//	wasMousePressed = isMousePressed;
//}

Vector3 Lerp(const Vector3& v1, const Vector3& v2, float t) {
	Vector3 v;

	v.x = t * v1.x + (1 - t) * v2.x;
	v.y = t * v1.y + (1 - t) * v2.y;
	v.z = t * v1.z + (1 - t) * v2.z;

	return v;
}

Vector3 ComputeBezierPoint(const Vector3& p0, const Vector3& p1, const Vector3& p2, float t) {
	Vector3 a = Lerp(p0, p1, t);
	Vector3 b = Lerp(p1, p2, t);
	return Lerp(a, b, t);
}

//void DrawBezier(const Vector3& controlPoint0, const Vector3& controlPoint1, const Vector3& controlPoint2,
//	const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color) {
//	const int maxIndex = 32;
//
//	for (int index = 0; index < maxIndex; index++) {
//		float t0 = index / float(maxIndex);
//		float t1 = (index + 1) / float(maxIndex);
//
//		//controlPoint0と1、1と2間の点を求める
//		Vector3 v0 = ComputeBezierPoint(controlPoint0, controlPoint1, controlPoint2, t0);
//		Vector3 v1 = ComputeBezierPoint(controlPoint0, controlPoint1, controlPoint2, t1);
//
//		//求めた点をスクリーン座標に変換
//		Vector3 screenV0 = Transform(Transform(v0, viewProjectionMatrix), viewportMatrix);
//		Vector3 screenV1 = Transform(Transform(v1, viewProjectionMatrix), viewportMatrix);
//
//		//求めた二つの点で曲線を描画
//		Novice::DrawLine(static_cast<int>(screenV0.x), static_cast<int>(screenV0.y),
//			static_cast<int>(screenV1.x), static_cast<int>(screenV1.y), color);
//	}
//}

Vector3 Reflect(const Vector3& input, Vector3& normal) {
	Vector3 reflect;

	normal = Normalize(normal);

	float dot = Dot(input, normal);

	reflect = input - normal * (2.0f * dot);
	return reflect;
}

float Deg2Rad(float deg) {
	float ret = static_cast<float>(deg) * (3.14159265358979323846f / 180.0f);
	return ret;
}
