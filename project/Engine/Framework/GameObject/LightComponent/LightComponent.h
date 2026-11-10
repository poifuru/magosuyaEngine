#pragma once
#include "Component.h"
#include "LightManager.h"
#include "struct.h"
#include <variant>
#include <type_traits>

//　--- 構造体の定義 --- //
// DirectionalLight
struct Directional {
	Vector4	color;		// 色
	float	intensity;	// 強度
	Vector3 direction;	// 向き
};

// PointLight
struct Point {
	Vector4 color;		// 色
	float	intensity;	// 強度
	Vector3 position;	// 位置
	float	radius;	//ライトが届く最大距離
	float	decay;		//減衰率
};

// SpotLight
struct Spot {
	Vector4 color;		// 色
	float	intensity;	// 強度
	Vector3 position;	// 位置
	Vector3 direction;	// 向き
	float	distance;	// ライトが届く最大距離
	float	decay;		// 減衰率
	float	cosAngle;	// スポットライトの余弦
};

// RectLight
struct Rect {
	Vector4 color;		// 色
	float	intensity;	// 強度
	Vector3 position;	// 位置
	Vector3 direction;	// 向き
	Vector2 size;		// 大きさ
	Vector3 right;		// 右ベクトル
	Vector3 up;		// 上ベクトル
	float	decay;		// 減衰率
};

// ライトのパラメータ（4種類のどれか1つだけを持つ）
// ※並び順は LightType（DIRECTIONALLIGHT, POINTLIGHT, SPOTLIGHT, RECTLIGHT）と揃えること
//   index() をそのまま LightType として使っている
using LightParam = std::variant<Directional, Point, Spot, Rect>;

class LightComponent : public Component {
public:
	LightComponent() = default;
	~LightComponent() override = default;

	void Initialize() override;
	void Update() override;
	void ImGui() override;

	void Serialize(json& j) const override;
	void Deserialize(const json& j) override;

	const char* GetName() const override { return "Light"; }

#pragma region アクセッサ (LightManagerがデータを集約するときに使う)
	// LightTypeを返す（variant が今どの型を持っているか = index()）
	LightType GetLightType() const { return static_cast<LightType>(param_.index()); }

	// パラメータを丸ごと返す（LightManager が std::visit で読む）
	const LightParam& GetParam() const { return param_; }

	// Colorを返す
	Vector4 GetColor() const;
	// Colorをセット
	void SetColor(const Vector4& color);

	// Intensityを返す
	float GetIntensity() const;
	// Intensityをセット
	void SetIntensity(const float& intensity);

	// Radiusを返す
	float GetRadius() const;
	// Radiusをセット
	void SetRadius(const float& radius);

	// Distanceを返す
	float GetDistance() const;
	// Distanceをセット
	void SetDistance(const float& distance);

	// CosAngleを返す
	float GetCosAngle() const;
	// CosAngleをセット
	void SetCosAngle(const float& cosAngle);

	// Sizeを返す
	Vector2 GetSize() const;
	// Sizeをセット
	void SetSize(const Vector2& size);
#pragma endregion

private:
	// 種類ごとの初期値を返す（index は LightType の値）
	static LightParam MakeDefaultParam(size_t index);

private:
	// 種類は param_.index() で分かるので、別に type_ は持たない（ズレる心配がない）
	LightParam param_ = MakeDefaultParam(DIRECTIONALLIGHT);
	bool isInitialized_ = false;
};
