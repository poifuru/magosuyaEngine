#pragma once
#include "struct.h"
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
float Length(const Vector3& v);

//正規化
Vector3 Normalize(const Vector3& v);

//クロス積
Vector3 Cross(const Vector3& v1, const Vector3& v2);

//Matrix4x4 * Vector3の乗算
Vector4 Multiply(const Matrix4x4& mat, const Vector4& vec);

//4x4行列関数
//加法
Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);

//減法
Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);

//積
float Minor(const Matrix4x4& mat, int row, int col);
float Determinant(const Matrix4x4& mat);
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

//演算子オーバーロード
Vector3 operator+(const Vector3& v1, const Vector3& v2);
Vector3 operator-(const Vector3& v1, const Vector3& v2);
Vector3 operator*(float s, const Vector3& v);
Vector3 operator*(const Vector3& v, float s);
Vector3 operator/(const Vector3& v, float s);
Matrix4x4 operator+(const Matrix4x4& m1, const Matrix4x4& m2);
Matrix4x4 operator-(const Matrix4x4& m1, const Matrix4x4& m2);
Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4& m2);
/*単項演算子*/
Vector3 operator+(const Vector3& v);
Vector3 operator-(const Vector3& v);

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

//透視投影行列
Matrix4x4 MakePerspectiveFOVMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

//正射影行列
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

//ビューポート変換行列
Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);

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

//AABBと球の当たり判定
bool IsCollision(const AABB& aabb, const Sphere& sphere);

//AABBと線分の当たり判定
bool IsCollision(const AABB& aabb, const Segment& segment);

//オブジェクトの回転を操作する関数
//void objectRotation(Vector3& rotate);

//Vector3Lerp関数
Vector3 Lerp(const Vector3& v1, const Vector3& v2, float t);

//ベジェ曲線描画用の点を求める関数
Vector3 ComputeBezierPoint(const Vector3& p0, const Vector3& p1, const Vector3& p2, float t);

//ベジェ曲線描画関数
//void DrawBezier(const Vector3& controlPoint0, const Vector3& controlPoint1, const Vector3& controlPoint2,
//	const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color);

//反射ベクトルを求める関数
Vector3 Reflect(const Vector3& input, Vector3& normal);