#include "PCH.h"
#include "MathFunction.h"
#include "Geometry.h"

namespace Math {
	Vector3 Add (const Vector3& v1, const Vector3& v2) {
		return {
			v1.x + v2.x,
			v1.y + v2.y,
			v1.z + v2.z
		};
	}

	Vector3 Subtract (const Vector3& v1, const Vector3& v2) {
		return {
			v1.x - v2.x,
			v1.y - v2.y,
			v1.z - v2.z
		};
	}

	Vector3 Multiply (float scalar, const Vector3& v) {
		return {
			scalar * v.x,
			scalar * v.y,
			scalar * v.z
		};
	}

	float Dot (const Vector3& v1, const Vector3& v2) {
		return {
			(v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z)
		};
	}

	float Length (const Vector3& v) {
		return {
			sqrtf (powf (v.x, 2) + powf (v.y, 2) + powf (v.z, 2))
		};
	}

	Vector3 Normalize (const Vector3& v) {
		float length = Length(v);

		if (length <= std::numeric_limits<float>::epsilon()) {
			return {};
		}

		return v / length;
	}

	Vector3 Cross (const Vector3& v1, const Vector3& v2) {
		Vector3 result = {
			v1.y * v2.z - v1.z * v2.y,
			v1.z * v2.x - v1.x * v2.z,
			v1.x * v2.y - v1.y * v2.x
		};

		return result;
	}

	Vector4 Multiply (const Matrix4x4& mat, const Vector4& vec) {
		Vector4 result = {};

		result.x = mat.m[0][0] * vec.x + mat.m[0][1] * vec.y + mat.m[0][2] * vec.z + mat.m[0][3] * vec.w;
		result.y = mat.m[1][0] * vec.x + mat.m[1][1] * vec.y + mat.m[1][2] * vec.z + mat.m[1][3] * vec.w;
		result.z = mat.m[2][0] * vec.x + mat.m[2][1] * vec.y + mat.m[2][2] * vec.z + mat.m[2][3] * vec.w;
		result.w = mat.m[3][0] * vec.x + mat.m[3][1] * vec.y + mat.m[3][2] * vec.z + mat.m[3][3] * vec.w;

		return result;
	}

	bool isClose(const Vector2& a, const Vector2 b, float epsilon) {
		return (std::abs(a.x - b.x) < epsilon) &&
			(std::abs(a.y - b.y) < epsilon);
	}

	bool isClose(const Vector3& a, const Vector3 b, float epsilon) {
		return (std::abs(a.x - b.x) < epsilon) &&
			(std::abs(a.y - b.y) < epsilon) &&
			(std::abs(a.z - b.z) < epsilon);
	}

	bool isClose(const Vector4& a, const Vector4 b, float epsilon) {
		return (std::abs(a.x - b.x) < epsilon) &&
			(std::abs(a.y - b.y) < epsilon) &&
			(std::abs(a.z - b.z) < epsilon) &&
			(std::abs(a.w - b.w) < epsilon);
	}

	Matrix4x4 Add (const Matrix4x4& m1, const Matrix4x4& m2) {
		Matrix4x4 result = {};
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				result.m[i][j] = m1.m[i][j] + m2.m[i][j];
			}
		}
		return result;
	}

	Matrix4x4 Subtract (const Matrix4x4& m1, const Matrix4x4& m2) {
		Matrix4x4 result = {};
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				result.m[i][j] = m1.m[i][j] - m2.m[i][j];
			}
		}
		return result;
	}

	Matrix4x4 Multiply (const Matrix4x4& m1, const Matrix4x4& m2) {
		Matrix4x4 result = {};

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

	//逆行列補助用の関数
	float Minor (const Matrix4x4& mat, int row, int col) {
		float sub[3][3] = {};
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
	float Determinant (const Matrix4x4& mat) {
		float det = 0.0f;
		for (int col = 0; col < 4; ++col) {
			float sign = (col % 2 == 0) ? 1.0f : -1.0f;
			det += sign * mat.m[0][col] * Minor (mat, 0, col);
		}
		return det;
	}

	Matrix4x4 Inverse (const Matrix4x4& m) {
		Matrix4x4 result = {};
		float det = Determinant (m);

		// ゼロ除算防止（特異行列）
		if (std::fabs (det) < 1e-6f) {
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
				result.m[j][i] = sign * Minor (m, i, j); // 転置して代入（adjugated）
			}
		}

		// 行列式で割る（スカラー倍）
		float invDet = 1.0f / det;
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				result.m[i][j] *= invDet;

		return result;
	}

	Matrix4x4 Transpose (const Matrix4x4& m) {
		Matrix4x4 result = {};
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				result.m[i][j] = m.m[j][i];
			}
		}
		return result;
	}

	Matrix4x4 MakeIdentity4x4 () {
		Matrix4x4 result = { 0.0f };

		for (int i = 0; i < 4; ++i) {
			result.m[i][i] = 1.0f; // 対角成分だけ 1 にする
		}

		return result;
	}

	Matrix4x4 MakeTranslateMatrix (const Vector3& translate) {
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

	Matrix4x4 MakeScaleMatrix (const Vector3& scale) {
		Matrix4x4 result = { 0.0f };

		result.m[0][0] = scale.x;
		result.m[1][1] = scale.y;
		result.m[2][2] = scale.z;
		result.m[3][3] = 1.0f;

		return result;
	}

	Vector3 ChangeTransform (const Vector3& vector, const Matrix4x4& matrix) {
		Vector3 result = {};

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

	Matrix4x4 MakeRotateXMatrix (float radian) {
		Matrix4x4 result = MakeIdentity4x4 ();

		result.m[1][1] = cosf (radian);
		result.m[1][2] = sinf (radian);
		result.m[2][1] = -sinf (radian);
		result.m[2][2] = cosf (radian);

		return result;
	}

	Matrix4x4 MakeRotateYMatrix (float radian) {
		Matrix4x4 result = MakeIdentity4x4 ();

		result.m[0][0] = cosf (radian);
		result.m[0][2] = -sinf (radian);
		result.m[2][0] = sinf (radian);
		result.m[2][2] = cosf (radian);

		return result;
	}

	Matrix4x4 MakeRotateZMatrix (float radian) {
		Matrix4x4 result = MakeIdentity4x4 ();

		result.m[0][0] = cosf (radian);
		result.m[0][1] = sinf (radian);
		result.m[1][0] = -sinf (radian);
		result.m[1][1] = cosf (radian);

		return result;
	}

	Matrix4x4 MakeAffineMatrix (const Vector3& scale, const Vector3& rotate, const Vector3& translate) {
		Matrix4x4 matrix = MakeIdentity4x4 ();

		Matrix4x4 scaleMatrix = Multiply (matrix, MakeScaleMatrix (scale));
		Matrix4x4 rotateXMatrix = Multiply (scaleMatrix, MakeRotateXMatrix (rotate.x));
		Matrix4x4 rotateXYMatrix = Multiply (rotateXMatrix, MakeRotateYMatrix (rotate.y));
		Matrix4x4 rotateXYZMatrix = Multiply (rotateXYMatrix, MakeRotateZMatrix (rotate.z));
		Matrix4x4 translateMatrix = Multiply (rotateXYZMatrix, MakeTranslateMatrix (translate));

		return translateMatrix;
	}

	Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Quaternion& rotate, const Vector3& translate) {
		Matrix4x4 result = MakeIdentity4x4();

		Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);
		Matrix4x4 rotateMatrix = MakeRotateMatrix(rotate);
		Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

		result = Multiply(Multiply(scaleMatrix, rotateMatrix), translateMatrix);

		return result;
	}

	Matrix4x4 MakePerspectiveFOVMatrix (float fovY, float aspectRatio, float nearClip, float farClip) {
		Matrix4x4 result = { 0.0f };

		result.m[0][0] = (1 / aspectRatio) * (1 / tanf (fovY / 2));
		result.m[1][1] = 1 / tanf (fovY / 2);
		result.m[2][2] = farClip / (farClip - nearClip);
		result.m[2][3] = 1.0f;
		result.m[3][2] = (-nearClip * farClip) / (farClip - nearClip);

		return result;
	}

	Matrix4x4 MakeOrthographicMatrix (float left, float top, float right, float bottom, float nearClip, float farClip) {
		Matrix4x4 result = MakeIdentity4x4 ();

		result.m[0][0] = 2 / (right - left);
		result.m[1][1] = 2 / (top - bottom);
		result.m[2][2] = 1 / (farClip - nearClip);
		result.m[3][0] = (left + right) / (left - right);
		result.m[3][1] = (top + bottom) / (bottom - top);
		result.m[3][2] = nearClip / (nearClip - farClip);

		return result;
	}

	Matrix4x4 MakeViewportMatrix (float left, float top, float width, float height, float minDepth, float maxDepth) {
		Matrix4x4 result = MakeIdentity4x4 ();

		result.m[0][0] = (width / 2);
		result.m[1][1] = -(height / 2);
		result.m[2][2] = maxDepth - minDepth;
		result.m[3][0] = left + (width / 2);
		result.m[3][1] = top + (height / 2);
		result.m[3][2] = minDepth;

		return result;
	}

	Matrix4x4 MakeLookAtMatrix (const Vector3& eye, const Vector3& target, const Vector3& up) {
		//前方向ベクトルfの計算(Z軸)
		Vector3 f = Math::Normalize (Math::Subtract (target, eye)); // f = normalize(target - eye)

		//右方向ベクトルrの計算(X軸)
		//fとUpの外積を取る
		Vector3 r = Math::Normalize (Math::Cross (up, f)); // r = normalize(Up x f)

		//上方向ベクトルuの計算(Y軸)
		//fとrの外積を取る
		Vector3 u = Math::Cross (f, r);

		//f,r,uを使ってビュー行列M_viewを構築する
		// ビュー行列は、回転と平行移動を兼ねた行列でワールド座標をカメラ座標へ変換する

		// 平行移動成分tの計算(内積を使う)
		float t_x = -Math::Dot (r, eye);
		float t_y = -Math::Dot (u, eye);
		float t_z = -Math::Dot (f, eye);

		Matrix4x4 result = {};

		//X軸(右ベクトル r)
		result.m[0][0] = r.x;
		result.m[0][1] = r.y;
		result.m[0][2] = r.z;
		result.m[0][3] = t_x; //-r・e

		//Y軸(上ベクトル u)
		result.m[1][0] = u.x;
		result.m[1][1] = u.y;
		result.m[1][2] = u.z;
		result.m[1][3] = t_y; //-u・e

		//Z軸(前ベクトル f)
		result.m[2][0] = f.x;
		result.m[2][1] = f.y;
		result.m[2][2] = f.z;
		result.m[2][3] = t_z; //-f・e

		//W成分
		result.m[3][0] = 0.0f;
		result.m[3][1] = 0.0f;
		result.m[3][2] = 0.0f;
		result.m[3][3] = 1.0f;

		return result;
	}

	Vector3 Transform (const Vector3& v, const Matrix4x4& m) {
		Vector3 result = {};

		// v.x の分だけ M の X軸を進める
		result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + 1.0f * m.m[3][0];
		result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + 1.0f * m.m[3][1];
		result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + 1.0f * m.m[3][2];

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

	Vector3 Project (const Vector3& v1, const Vector3& v2) {
		Vector3 result{};

		float dot = Dot (v1, v2);
		float length = Length (v2);

		if (length != 0.0f) {
			result.x = (dot / powf (length, 2)) * v2.x;
			result.y = (dot / powf (length, 2)) * v2.y;
			result.z = (dot / powf (length, 2)) * v2.z;
		};

		return result;
	}

	Vector3 ClosestPoint (const Vector3& point, const Segment& segment) {
		// 始点から終点へのベクトル ab
		Vector3 ab = Subtract (segment.endpoint, segment.origin);
		Vector3 ap = Subtract (point, segment.origin);

		// ab の長さの2乗
		float lenSq = Dot(ab, ab);

		// ゼロ長Segment（始点 == 終点）のチェック
		if (lenSq <= 1e-6f) { // 1e-6f は極小値（イプシロン）
			return segment.origin; // 線分じゃなくて点なので、最接近点は始点そのもの
		}

		// 通常の射影パラメータ t の計算
		float t = Dot(ap, ab) / lenSq;

		// 線分なので [0, 1] の範囲にクランプ
		if (t < 0.0f) {
			return segment.origin;
		}
		if (t > 1.0f) {
			return segment.endpoint;
		}
		// 射影した点を計算して返す
		return Add(segment.origin, Multiply(t, ab));
	}

	bool IsCollision (const Sphere& s1, const Sphere& s2) {
		float dis = Length (Subtract (s1.center, s2.center));
		float radiusDis = s1.radius + s2.radius;

		return dis <= radiusDis;
	}

	bool IsCollision (const Sphere& sphere, const Plane& plane) {
		float k = Dot (plane.normal, sphere.center) - plane.distance;
		return fabs (k) <= sphere.radius;
	}

	Vector3 Perpendicular (const Vector3& vector) {
		if (vector.x != 0.0f || vector.y != 0.0f) {
			return { -vector.y, vector.x, 0.0f };
		}
		return { 0.0f, -vector.z, vector.y };
	}

	bool IsCollision (const Segment& segment, const Plane& plane) {
		float dot = Dot (plane.normal, segment.GetDiff());

		if (dot == 0.0f) {
			return false;
		}

		//tを求める
		float t = (plane.distance - Dot (segment.origin, plane.normal)) / dot;

		//tの値と線の種類によって衝突しているか判定する
		return t > 0.0f && t < 1.0f;
	}

	bool IsCollision (const Triangle& triangle, const Segment& segment) {
		Vector3 v0 = triangle.vertices[0];
		Vector3 v1 = triangle.vertices[1];
		Vector3 v2 = triangle.vertices[2];

		Vector3 edge0 = Subtract (v1, v0);
		Vector3 edge1 = Subtract (v2, v1);
		Vector3 edge2 = Subtract (v0, v2);

		// 法線ベクトル
		Vector3 normal = Cross (edge0, Subtract (v2, v0));

		// セグメントと平面の交差チェック
		float dot = Dot (normal, segment.GetDiff());
		if (dot == 0.0f) {
			return false;
		}

		float t = (Dot (normal, v0) - Dot (segment.origin, normal)) / dot;
		if (t < 0.0f || t > 1.0f) {
			return false;
		}

		// 交点を求める
		Vector3 P = Add (segment.origin, Multiply (t, segment.GetDiff()));

		// 三角形内にあるかチェック
		Vector3 c0 = Subtract (P, v0);
		Vector3 c1 = Subtract (P, v1);
		Vector3 c2 = Subtract (P, v2);

		if (Dot (Cross (edge0, c0), normal) >= 0.0f &&
			Dot (Cross (edge1, c1), normal) >= 0.0f &&
			Dot (Cross (edge2, c2), normal) >= 0.0f) {
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

	bool IsCollision (const AABB& aabb1, const AABB& aabb2) {
		return
			(aabb1.min.x <= aabb2.max.x && aabb1.max.x >= aabb2.min.x) &&
			(aabb1.min.y <= aabb2.max.y && aabb1.max.y >= aabb2.min.y) &&
			(aabb1.min.z <= aabb2.max.z && aabb1.max.z >= aabb2.min.z);
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

	bool IsCollision (const AABB& aabb, const Vector3& point) {
		return
			(aabb.min.x <= point.x && aabb.max.x >= point.x) &&
			(aabb.min.y <= point.y && aabb.max.y >= point.y) &&
			(aabb.min.z <= point.z && aabb.max.z >= point.z);
	}

	bool IsCollision (const AABB& aabb, const Sphere& sphere) {
		//最近接点を求める
		Vector3 closestPoint{
			std::clamp (sphere.center.x, aabb.min.x, aabb.max.x),
			std::clamp (sphere.center.y, aabb.min.y, aabb.max.y),
			std::clamp (sphere.center.z, aabb.min.z, aabb.max.z)
		};
		//最近接点と球のの中心との距離を求める//
		//球の中心と最近接点との差分ベクトル
		Vector3 vector = Subtract (sphere.center, closestPoint);
		float distance = Length (vector);

		return distance <= sphere.radius;
	}

	bool IsCollision (const AABB& aabb, const Segment& segment) {
		Vector3 dir = Subtract (segment.endpoint, segment.origin);
		float tmin = 0.0f;
		float tmax = 1.0f;

		// 各軸ごとに判定
		for (int i = 0; i < 3; ++i) {
			float origin = (&segment.origin.x)[i];
			float direction = (&dir.x)[i];
			float min = (&aabb.min.x)[i];
			float max = (&aabb.max.x)[i];

			if (fabs (direction) < 1e-6f) {
				// 線分がこの軸に平行 → AABBの内側にあるか
				if (origin < min || origin > max) {
					return false;
				}
			}
			else {
				float invD = 1.0f / direction;
				float t1 = (min - origin) * invD;
				float t2 = (max - origin) * invD;

				if (t1 > t2) std::swap (t1, t2);

				tmin = std::max(tmin, t1);
				tmax = std::min(tmax, t2);

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

	/*float Lerp(const float& start, const float& end, float t) {
		return t * end + (1 - t) * start;
	}

	Vector3 Lerp(const Vector3& start, const Vector3& end, float t) {
		Vector3 v = {};

		v.x = t * end.x + (1 - t) * start.x;
		v.y = t * end.y + (1 - t) * start.y;
		v.z = t * end.z + (1 - t) * start.z;

		return v;
	}*/

	Vector3 ComputeBezierPoint (const Vector3& p0, const Vector3& p1, const Vector3& p2, float t) {
		Vector3 a = Lerp (p0, p1, t);
		Vector3 b = Lerp (p1, p2, t);
		return Lerp (a, b, t);
	}

	Vector3 CatmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) {
		float t2 = t * t;
		float t3 = t2 * t;

		Vector3 result =
			p1 * 2.0f +
			(p2 - p0) * t +
			(p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 +
			(p0 * -1.0f + p1 * 3.0f - p2 * 3.0f + p3) * t3;

		return result * 0.5f;
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

	Vector3 Reflect (const Vector3& input, Vector3& normal) {
		Vector3 reflect;

		normal = Normalize (normal);

		float dot = Dot (input, normal);

		reflect = input - normal * (2.0f * dot);
		return reflect;
	}

	float Deg2Rad (float deg) {
		float ret = static_cast<float>(deg) * (3.14159265358979323846f / 180.0f);
		return ret;
	}

	Matrix4x4 MakeRotateAxisAngle(const Vector3& axis, float angle) {
		Matrix4x4 result = { 0.0f }; // 全て0で初期化
		float c = cosf(angle);
		float s = sinf(angle);
		float omc = 1.0f - c; // One Minus Cos

		// スライドの行列式 R に基づいて各要素を計算
		result.m[0][0] = axis.x * axis.x * omc + c;
		result.m[0][1] = axis.x * axis.y * omc + axis.z * s;
		result.m[0][2] = axis.x * axis.z * omc - axis.y * s;

		result.m[1][0] = axis.x * axis.y * omc - axis.z * s;
		result.m[1][1] = axis.y * axis.y * omc + c;
		result.m[1][2] = axis.y * axis.z * omc + axis.x * s;

		result.m[2][0] = axis.x * axis.z * omc + axis.y * s;
		result.m[2][1] = axis.y * axis.z * omc - axis.x * s;
		result.m[2][2] = axis.z * axis.z * omc + c;

		result.m[3][3] = 1.0f; // 同次座標
		return result;
	}

	Matrix4x4 DirectionToDirection(const Vector3& from, const Vector3& to) {
		Vector3 fromN = Normalize(from);
		Vector3 toN = Normalize(to);
		float cosTheta = Dot(fromN, toN);

		// 同じ方向
		if(cosTheta > 0.999999f) return MakeIdentity4x4();

		// 真逆の方向
		if(cosTheta < -0.999999f) {
			Vector3 axis;
			if(fabsf(fromN.x) > 1e-6f || fabsf(fromN.y) > 1e-6f) {
				axis = Normalize(Vector3{ fromN.y, -fromN.x, 0.0f });
			}
			else {
				axis = Normalize(Vector3{ fromN.z, 0.0f, -fromN.x });
			}
			return MakeRotateAxisAngle(axis, static_cast<float>(M_PI));
		}

		// 通常時
		Vector3 axis = Normalize(Cross(fromN, toN));
		float angle = acosf(cosTheta); // cosThetaから角度を求める
		return MakeRotateAxisAngle(axis, angle);
	}

	Quaternion Multiply(const Quaternion& lhs, const Quaternion& rhs) {
		return Quaternion(
			lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y,
			lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.x,
			lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w,
			lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z
		);
	}

	Quaternion IdentityQuaternion() {
		return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
	}

	Quaternion Conjugate(const Quaternion& q) {
		return Quaternion(-q.x, -q.y, -q.z, q.w);
	}

	float Norm(const Quaternion& q) {
		return std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
	}

	Quaternion Normalize(const Quaternion& q) {
		float n = Norm(q);
		if(n > 0.0f) {
			float invN = 1.0f / n;
			return Quaternion(q.x * invN, q.y * invN, q.z * invN, q.w * invN);
		}
		return IdentityQuaternion();
	}

	Quaternion Inverse(const Quaternion& q) {
		float n2 = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
		if(n2 > 0.0f) {
			float invN2 = 1.0f / n2;
			Quaternion conj = Conjugate(q);
			return Quaternion(conj.x * invN2, conj.y * invN2, conj.z * invN2, conj.w * invN2);
		}
		return IdentityQuaternion();
	}

	float Dot(const Quaternion& q0, const Quaternion& q1) {
		return q0.x * q1.x + q0.y * q1.y + q0.z * q1.z + q0.w * q1.w;
	}

	Quaternion MakeRotateAxisAngleQuaternion(const Vector3& axis, float angle) {
		Vector3 normAxis = Normalize(axis);
		float halfAngle = angle / 2.0f;
		float sinHalf = sinf(halfAngle);

		return Quaternion(
			normAxis.x * sinHalf,
			normAxis.y * sinHalf,
			normAxis.z * sinHalf,
			cosf(halfAngle)
		);
	}

	Vector3 RotateVector(const Vector3& vector, const Quaternion& quaternion) {
		//ベクトルをQuaternion形式 (x, y, z, 0) に変換
		Quaternion v(vector.x, vector.y, vector.z, 0.0f);

		//q * v * conjugate(q) を計算
		Quaternion conj = Conjugate(quaternion);
		Quaternion resultQ = Multiply(Multiply(quaternion, v), conj);

		//結果の虚部をVector3として返す
		return { resultQ.x, resultQ.y, resultQ.z };
	}

	Matrix4x4 MakeRotateMatrix(const Quaternion& quaternion) {
		Matrix4x4 result = MakeIdentity4x4();

		float x = quaternion.x;
		float y = quaternion.y;
		float z = quaternion.z;
		float w = quaternion.w;

		//Quaternionから回転行列への変換公式
		result.m[0][0] = 1.0f - 2.0f * y * y - 2.0f * z * z;
		result.m[0][1] = 2.0f * x * y + 2.0f * w * z;
		result.m[0][2] = 2.0f * x * z - 2.0f * w * y;

		result.m[1][0] = 2.0f * x * y - 2.0f * w * z;
		result.m[1][1] = 1.0f - 2.0f * x * x - 2.0f * z * z;
		result.m[1][2] = 2.0f * y * z + 2.0f * w * x;

		result.m[2][0] = 2.0f * x * z + 2.0f * w * y;
		result.m[2][1] = 2.0f * y * z - 2.0f * w * x;
		result.m[2][2] = 1.0f - 2.0f * x * x - 2.0f * y * y;

		return result;
	}

	Quaternion Slerp(const Quaternion& q0, const Quaternion& q1, float t) {
		float dot = Dot(q0, q1);

		//向きを補正するためのコピー
		Quaternion targetQ1 = q1;

		//内積が負の場合は逆向きに補間する
		if(dot < 0.0f) {
			targetQ1 = -q1;
			dot = -dot;
		}

		//ドット積が1に近い（角度がほぼ0）場合は、ゼロ除算を避けるために線形補間（Lerp）に切り替えるのが安全
		if(dot > 0.9995f) {
			//線形補間して正規化
			Quaternion result = q0 * (1.0f - t) + targetQ1 * t;
			return Normalize(result);
		}

		//なす角thetaを求める
		float theta_0 = std::acos(dot);   //q0とq1の間の角度
		float theta = theta_0 * t;     //補間後の角度

		//補間係数 scale0, scale1 を求める
		//公式: Slerp(q0, q1, t) = (sin((1-t)theta)/sin(theta)) * q0 + (sin(t*theta)/sin(theta)) * q1
		float sin_theta_0 = std::sin(theta_0);
		float sin_theta = std::sin(theta);

		float scale0 = std::sin(theta_0 - theta) / sin_theta_0;
		float scale1 = sin_theta / sin_theta_0;

		//それぞれの補間係数を利用して補間後のQuaternionを求める
		return q0 * scale0 + targetQ1 * scale1;
	}

	Quaternion Lerp(const Quaternion& start, const Quaternion& end, float t) {
		return Slerp(start, end, t);
	}
}