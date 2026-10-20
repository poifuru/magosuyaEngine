#pragma once
#include "Geometry.h"

namespace Math {
	static const int kColumnWidth = 60;
	static const int kRowHeight = 20;

	//加算
	Vector3 Add(const Vector3& v1, const Vector3& v2);

	//減算
	Vector3 Subtract(const Vector3& v1, const Vector3& v2);

	//乗算
	Vector3 Multiply(float scalar, const Vector3& v);

	//内積
	float Dot(const Vector3& v1, const Vector3& v2);

	//長さ(ノルム)
	float Length(const Vector2& v);
	float Length(const Vector3& v);

	//正規化
	Vector2 Normalize(const Vector2& v);
	Vector3 Normalize(const Vector3& v);

	//クロス積
	Vector3 Cross(const Vector3& v1, const Vector3& v2);

	//Matrix4x4 * Vector3の乗算
	Vector4 Multiply(const Matrix4x4& mat, const Vector4& vec);

	// float型のまるめ誤差に対応した、等しいかを判定する関数
	bool isClose(const Vector2& a, const Vector2 b, float epsilon = 1e-5f);

	bool isClose(const Vector3& a, const Vector3 b, float epsilon = 1e-5f);

	bool isClose(const Vector4& a, const Vector4 b, float epsilon = 1e-5f);

	//4x4行列関数
	//加法
	Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);

	//減法
	Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);

	//積
	float Minor(const Matrix4x4& mat, int row, int col);
	float Determinant(const Matrix4x4& mat);
	Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

	//逆行列
	Matrix4x4 Inverse(const Matrix4x4& m);

	//転置行列
	Matrix4x4 Transpose(const Matrix4x4& m);

	//単位行列の作成
	Matrix4x4 MakeIdentity4x4();

	//平行移動行列
	Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

	//拡大縮小行列
	Matrix4x4 MakeScaleMatrix(const Vector3& scale);

	//座標変換
	Vector3 ChangeTransform(const Vector3& vector, const Matrix4x4& matrix);

	//X軸回転行列
	Matrix4x4 MakeRotateXMatrix(float radian);

	//Y軸回転行列
	Matrix4x4 MakeRotateYMatrix(float radian);

	//Z軸回転行列
	Matrix4x4 MakeRotateZMatrix(float radian);

	//3次元アフィン変換行列
	Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

	//3次元アフィン変換行列(Quaternion)
	Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Quaternion& rotate, const Vector3& translate);

	//透視投影行列
	Matrix4x4 MakePerspectiveFOVMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

	//正射影行列
	Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

	//ビューポート変換行列
	Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);

	//LookAt行列
	Matrix4x4 MakeLookAtMatrix(const Vector3& eye, const Vector3& target, const Vector3& up);

	Vector3 Transform(const Vector3& v, const Matrix4x4& m);

	//Vector3数値表示
	//void VectorScreenPrintf(int x, int y, const Vector3& vector, const char* label);

	//4x4行列の数値表示
	//void MatrixScreenPrint(int x, int y, const Matrix4x4& m, const char* str);

	//Grid描画関数
	//void DrawGrid(const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix);

	//球を描画する関数
	//void DrawSphere(const Vector3& center, const float& radius, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color);

	//正射影ベクトルを求める関数
	Vector3 Project(const Vector3& v1, const Vector3& v2);

	//最近接点を求める関数
	Vector3 ClosestPoint(const Vector3& point, const Segment& segment);

	//球同士の当たり判定
	bool IsCollision(const Sphere& s1, const Sphere& s2);

	//球と平面の当たり判定
	bool IsCollision(const Sphere& sphere, const Plane& plane);

	//頂点を求める関数
	Vector3 Perpendicular(const Vector3& vector3);

	//平面を描画する関数
	//void DrawPlane(const Plane& plane, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color);

	/*線と平面の当たり判定*/
	//直線
	//bool IsCollision(const Line& line, const Plane& plane);

	//半直線
	//bool IsCollision(const Ray& ray, const Plane& plane);

	//線分
	bool IsCollision(const Segment& segment, const Plane& plane);

	//三角形と線分の当たり判定
	bool IsCollision(const Triangle& triangle, const Segment& segment);

	//三角形描画の関数
	//void DrawTriangle(const Triangle& triangle, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color);

	//AABBの当たり判定
	bool IsCollision(const AABB& aabb1, const AABB& aabb2);

	//AABBの描画
	//void DrawAABB(const AABB& aabb, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color);

	//AABBと点の当たり判定
	bool IsCollision(const AABB& aabb, const Vector3& point);

	//AABBと球の当たり判定
	bool IsCollision(const AABB& aabb, const Sphere& sphere);

	//AABBと線分の当たり判定
	bool IsCollision(const AABB& aabb, const Segment& segment);

	//オブジェクトの回転を操作する関数
	//void objectRotation(Vector3& rotate);

	//// floatLerp関数
	//float Lerp(const float& f1, const float& f2, float t);

	////Vector3Lerp関数
	//Vector3 Lerp(const Vector3& v1, const Vector3& v2, float t);

	//ベジェ曲線描画用の点を求める関数
	Vector3 ComputeBezierPoint(const Vector3& p0, const Vector3& p1, const Vector3& p2, float t);

	// Catmull-Rom スプライン
	Vector3 CatmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t);

	//ベジェ曲線描画関数
	//void DrawBezier(const Vector3& controlPoint0, const Vector3& controlPoint1, const Vector3& controlPoint2,
	//	const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color);

	//反射ベクトルを求める関数
	Vector3 Reflect(const Vector3& input, Vector3& normal);

	//DegreeをRadianで返す関数
	float Deg2Rad(float deg);

	//任意軸回転行列作成関数
	Matrix4x4 MakeRotateAxisAngle(const Vector3& axis, float angle);

	//ある軸からある軸に向けて回転させる関数
	Matrix4x4 DirectionToDirection(const Vector3& from, const Vector3& to);

	//Quaternionの積 (ハミルトン積)
	Quaternion Multiply(const Quaternion& lhs, const Quaternion& rhs);

	//単位Quaternionを返す
	Quaternion IdentityQuaternion();

	//共役Quaternionを返す
	Quaternion Conjugate(const Quaternion& q);

	//Quaternionのnorm（ノルム）を返す
	float Norm(const Quaternion& q);

	//正規化したQuaternionを返す
	Quaternion Normalize(const Quaternion& q);

	//逆Quaternionを返す
	Quaternion Inverse(const Quaternion& q);

	//Quaternionの内積
	float Dot(const Quaternion& q0, const Quaternion& q1);

	//任意回転軸を表すQuaternionの生成
	Quaternion MakeRotateAxisAngleQuaternion(const Vector3& axis, float angle);

	//ベクトルをQuaternionで回転させた結果のベクトルを求める
	Vector3 RotateVector(const Vector3& vector, const Quaternion& quaternion);

	//Quaternionから回転行列を求める
	Matrix4x4 MakeRotateMatrix(const Quaternion& quaternion);

	//球面線形補間
	Quaternion Slerp(const Quaternion& q0, const Quaternion& q1, float t);

	//Lerp関数(template対応)
	template<typename T>
	T Lerp(const T& start, const T& end, float t) {
		T result;
		// tが0の時にstart、1の時にendになる
		result = start + (end - start) * t;
		return result;
	}

	//Quaternionようにオーバーロード
	Quaternion Lerp(const Quaternion& start, const Quaternion& end, float t);
}