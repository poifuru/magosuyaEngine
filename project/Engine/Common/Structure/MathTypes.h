#pragma once

// Vector2 
struct Vector2 {
	float x;
	float y;

	// 足し算
	Vector2 operator+(const Vector2& other) const { return { x + other.x, y + other.y }; }

	// 引き算
	Vector2 operator-(const Vector2& other) const { return { x - other.x, y - other.y }; }

	// スカラー倍
	Vector2 operator*(float s) const { return { x * s, y * s }; }

	// スカラー除算
	Vector2 operator/(float s) const { return{ x / s, y / s }; }

	// 符号反転
	Vector2 operator-() const { return { -x, -y }; }

	// 比較演算子
	bool operator==(const Vector2& other) const { return (x == other.x) && (y == other.y); }
	bool operator!=(const Vector2& other) const { return !(*this == other); }

	/*複合代入演算子*/
	Vector2& operator*=(float s) { x *= s; y *= s; return *this; }
	Vector2& operator-=(const Vector2& v) { x -= v.x; y -= v.y; return *this; }
	Vector2& operator+=(const Vector2& v) { x += v.x; y += v.y; return *this; }
	Vector2& operator/=(float s) { x /= s; y /= s; return *this; }
};
// グローバル演算子 (s * v)
inline Vector2 operator*(float s, const Vector2& v) {
	return v * s;
}

// Vector3 
struct Vector3 {
	float x;
	float y;
	float z;

	// 足し算
	Vector3 operator+(const Vector3& other) const { return { x + other.x, y + other.y, z + other.z }; }

	// 引き算
	Vector3 operator-(const Vector3& other) const { return { x - other.x, y - other.y, z - other.z }; }

	// スカラー乗算
	Vector3 operator*(float s) const { return { x * s, y * s, z * s }; }

	// スカラー除算
	Vector3 operator/(float s) const { return{ x / s, y / s, z / s }; }

	// 符号反転
	Vector3 operator-() const { return { -x, -y, -z }; }

	// 比較演算子
	bool operator==(const Vector3& other) const { return (x == other.x) && (y == other.y) && (z == other.z); }
	bool operator!=(const Vector3& other) const { return !(*this == other); }

	/*複合代入演算子*/
	Vector3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
	Vector3& operator-=(const Vector3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	Vector3& operator+=(const Vector3& v) { x += v.x; y += v.y; z += v.z; return *this; }
	Vector3& operator/=(float s) { x /= s; y /= s; z /= s; return *this; }
};
// グローバル演算子 (s * v)
inline Vector3 operator*(float s, const Vector3& v) {
	return v * s;
}

// Vector4 
struct Vector4 {
	float x;
	float y;
	float z;
	float w;

	// 足し算
	Vector4 operator+(const Vector4& other) const { return { x + other.x, y + other.y, z + other.z, w + other.w }; }

	// 引き算
	Vector4 operator-(const Vector4& other) const { return { x - other.x, y - other.y, z - other.z, w - other.w }; }

	// スカラー倍
	Vector4 operator*(float s) const { return { x * s, y * s, z * s, w * s }; }

	// スカラー除算
	Vector4 operator/(float s) const { return{ x / s, y / s, z / s, w / s }; }

	// 符号反転
	Vector4 operator-() const { return { -x, -y, -z, -w }; }

	// 比較演算子
	bool operator==(const Vector4& other) const { return (x == other.x) && (y == other.y) && (z == other.z) && (w == other.w); }
	bool operator!=(const Vector4& other) const { return !(*this == other); }

	/*複合代入演算子*/
	Vector4& operator*=(float s) { x *= s; y *= s; z *= s; w *= s;  return *this; }
	Vector4& operator-=(const Vector4& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
	Vector4& operator+=(const Vector4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
	Vector4& operator/=(float s) { x /= s; y /= s; z /= s; w /= s; return *this; }
};
// グローバル演算子 (s * v)
inline Vector4 operator*(float s, const Vector4& v) {
	return v * s;
}

// 3x3Matrix 
struct Matrix3x3 {
	float m[3][3];

	// 行列同士の掛け算
	Matrix3x3 operator*(const Matrix3x3& other) const {
		Matrix3x3 result;
		for(int i = 0; i < 3; ++i) {
			for(int j = 0; j < 3; ++j) {
				result.m[i][j] = 
					m[i][0] * other.m[0][j] +
					m[i][1] * other.m[1][j] +
					m[i][2] * other.m[2][j];
			}
		}
		return result;
	}
};

// 4x4Matrix 
struct Matrix4x4 {
	float m[4][4];

	// 行列の加算
	Matrix4x4 operator+(const Matrix4x4& other) const {
		Matrix4x4 result;
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				result.m[i][j] = m[i][j] + other.m[i][j];
			}
		}
		return result;
	}
	// 行列の減算
	Matrix4x4 operator-(const Matrix4x4& other) const {
		Matrix4x4 result;
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				result.m[i][j] = m[i][j] - other.m[i][j];
			}
		}
		return result;
	}

	// 行列同士の掛け算
	Matrix4x4 operator*(const Matrix4x4& other) const {
		Matrix4x4 result;
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				result.m[i][j] = 
					m[i][0] * other.m[0][j] +
					m[i][1] * other.m[1][j] +
					m[i][2] * other.m[2][j] +
					m[i][3] * other.m[3][j];
			}
		}
		return result;
	}

	// 複合代入演算子
	Matrix4x4& operator*=(const Matrix4x4& other) {
		*this = *this * other;
		return *this;
	}
};
// グローバル演算子(v * m)
inline Vector4 operator*(const Vector4& v, const Matrix4x4& m) {
	return {
		v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + v.w * m.m[3][0],
		v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + v.w * m.m[3][1],
		v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + v.w * m.m[3][2],
		v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + v.w * m.m[3][3]
	};
}

// Quaternion 
struct Quaternion {
	float x;
	float y;
	float z;
	float w;

	// 基本的な演算子のオーバーロード
	Quaternion operator*(float s) const { return { x * s, y * s, z * s, w * s }; }
	Quaternion operator+(const Quaternion& q) const { return { x + q.x, y + q.y, z + q.z, w + q.w }; }
	Quaternion operator-() const { return { -x, -y, -z, -w }; }

	// Quaternion同士の乗算(回転の合成)
	Quaternion operator*(const Quaternion& other) const {
		return Quaternion(
			w * other.x + x * other.w + y * other.z - z * other.y,
			w * other.y - x * other.z + y * other.w + z * other.x,
			w * other.z + x * other.y - y * other.x + z * other.w,
			w * other.w - x * other.x - y * other.y - z * other.z
		);
	}

	// 複合代入演算子 (*=)
	Quaternion& operator*=(const Quaternion& other) {
		*this = *this * other;
		return *this;
	}
};

// ベクトルをクォータニオンで回転させる (q * v)
inline Vector3 operator*(const Quaternion& q, const Vector3& v) {
	// 外積 (q.vector x v) の 2倍を計算
	float tx = 2.0f * (q.y * v.z - q.z * v.y);
	float ty = 2.0f * (q.z * v.x - q.x * v.z);
	float tz = 2.0f * (q.x * v.y - q.y * v.x);
	// v + q.w * t + (q.vector x t) を計算して返す
	return {
		v.x + q.w * tx + (q.y * tz - q.z * ty),
		v.y + q.w * ty + (q.z * tx - q.x * tz),
		v.z + q.w * tz + (q.x * ty - q.y * tx)
	};
}