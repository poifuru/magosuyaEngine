#include "PCH.h"
#include "LightComponent.h"
#include "GameObject.h"
#include "MathFunction.h"

void LightComponent::Initialize() {
	if (isInitialized_) return;
	isInitialized_ = true;

	// デフォルトのDirectionalLightで初期化しておく
	type_ = DIRECTIONALLIGHT;

	Directional dir;
	dir.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	dir.intensity = 1.0f;
	dir.direction = { 0.0f, -1.0f, 0.0f };
	param_ = dir;
}

void LightComponent::Update() {
	GameObject* owner = GetGameObject();
	if(!owner) return;

	// Transformの取得
	auto& transform = owner->GetTransform();

	// パラメータを必要としているライトに同期させる 
	// 位置(Point, Spot, Rect)
	if(auto* p = std::get_if<Point>(&param_))      p->position = transform.translate;
	else if(auto* p = std::get_if<Spot>(&param_)) p->position = transform.translate;
	else if(auto* p = std::get_if<Rect>(&param_)) p->position = transform.translate;

	// 向き (Directional, Spot, Rect）
	// 各軸の回転から Quaternion を作成して合成する
	Quaternion qX = Math::MakeRotateAxisAngleQuaternion({1.0f, 0.0f, 0.0f}, transform.rotate.x);
	Quaternion qY = Math::MakeRotateAxisAngleQuaternion({0.0f, 1.0f, 0.0f}, transform.rotate.y);
	Quaternion qZ = Math::MakeRotateAxisAngleQuaternion({0.0f, 0.0f, 1.0f}, transform.rotate.z);
	Quaternion q = Math::Multiply(Math::Multiply(qX, qY), qZ);
	// Quaternion から回転行列を作る
	Matrix4x4 rotMat = Math::MakeRotateMatrix(q);

	if(auto* p = std::get_if<Directional>(&param_)) {
		p->direction = Math::Transform({ 0.0f, -1.0f, 0.0f }, rotMat); // デフォルト方向を回転
	}
	else if(auto* p = std::get_if<Spot>(&param_)) {
		p->direction = Math::Transform({ 0.0f, 0.0f, 1.0f }, rotMat);
	}
	else if(auto* p = std::get_if<Rect>(&param_)) {
		p->direction = Math::Transform({ 0.0f, 0.0f, 1.0f }, rotMat);
		p->right = Math::Transform({ 1.0f, 0.0f, 0.0f }, rotMat);
		p->up = Math::Transform({ 0.0f, 1.0f, 0.0f }, rotMat);
	}

	// 位置の同期
	Vector3 lightPosition = transform.translate;
}

void LightComponent::ImGui() {
	// ライトタイプの切り替え
	const char* types[] = { "平行光源", "点光源", "スポットライト", "面光源" };
	int currentType = static_cast<int>(type_);
	if(ImGui::Combo("種類", &currentType, types, IM_ARRAYSIZE(types))) {
		type_ = static_cast<LightType>(currentType);
		// タイプが切り替わったら variant の中身を再初期化する
		if(type_ == DIRECTIONALLIGHT) param_ = Directional{
			{1.0f,1.0f,1.0f,1.0f}, 1.0f, {0.0f,-1.0f,0.0f} 
		};
		else if(type_ == POINTLIGHT) param_ = Point{ 
			{1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, {0.0f, 0.0f, 0.0f }, 10.0f, 1.0f
		};
		else if(type_ == SPOTLIGHT) param_ = Spot{
			{1.0f, 1.0f, 1.0f, 1.0f }, 1.0f, {0.0f, 0.0f, 0.0f }, {0.0f, 0.0f, 1.0f }, 10.0f, 1.0f, 0.5f
		};
		else if(type_ == RECTLIGHT) param_ = Rect{
			{1.0f, 1.0f, 1.0f, 1.0f }, 1.0f, {0.0f, 0.0f, 0.0f }, {0.0f, 0.0f, 1.0f }, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f }, {0.0f, 1.0f, 0.0f }, 1.0f 
		};
	}

	ImGui::Separator();

	// 共通パラメータの編集
	Vector4 color = GetColor();
	if (ImGui::ColorEdit4("色", &color.x)) {
		SetColor(color);
	}
	float intensity = GetIntensity();
	if (ImGui::DragFloat("強さ", &intensity, 0.05f, 0.0f, 100.0f)) {
		SetIntensity(intensity);
	}

	// 個別パラメータの編集
	if (auto* p = std::get_if<Point>(&param_)) {
		ImGui::DragFloat("半径", &p->radius, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("減衰", &p->decay, 0.05f, 0.0f, 10.0f);
	}
	else if (auto* p = std::get_if<Spot>(&param_)) {
		ImGui::DragFloat("距離", &p->distance, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("減衰", &p->decay, 0.05f, 0.0f, 10.0f);
		// コサインだと角度の感覚が掴みづらいので、度数法(0~180度)に変換して編集できるようにする
		float angle = acosf(p->cosAngle) * (180.0f / 3.14159265f) * 2.0f;
		if (ImGui::DragFloat("角度", &angle, 0.5f, 0.0f, 180.0f)) {
			p->cosAngle = cosf((angle * 0.5f) * (3.14159265f / 180.0f));
		}
	}
	else if (auto* p = std::get_if<Rect>(&param_)) {
		ImGui::DragFloat2("大きさ", &p->size.x, 0.05f, 0.0f, 50.0f);
		ImGui::DragFloat("減衰", &p->decay, 0.05f, 0.0f, 10.0f);
	}
}

void LightComponent::Serialize(json& j) const {
	j["type"] = "LightComponent";
	j["lightType"] = static_cast<int>(type_);
	j["color"] = { GetColor().x, GetColor().y, GetColor().z, GetColor().w };
	j["intensity"] = GetIntensity();
	if (auto* p = std::get_if<Point>(&param_)) {
		j["radius"] = p->radius;
		j["decay"] = p->decay;
	}
	else if (auto* p = std::get_if<Spot>(&param_)) {
		j["distance"] = p->distance;
		j["decay"] = p->decay;
		j["cosAngle"] = p->cosAngle;
	}
	else if (auto* p = std::get_if<Rect>(&param_)) {
		j["size"] = { p->size.x, p->size.y };
		j["decay"] = p->decay;
	}
}

void LightComponent::Deserialize(const json& j) {
	isInitialized_ = true;
	type_ = static_cast<LightType>(j["lightType"]);
	Vector4 color = { j["color"][0], j["color"][1], j["color"][2], j["color"][3] };
	float intensity = j["intensity"];
	if (type_ == DIRECTIONALLIGHT) {
		param_ = Directional{ color, intensity, {0,-1,0} };
	}
	else if (type_ == POINTLIGHT) {
		param_ = Point{ color, intensity, {0,0,0}, j["radius"], j["decay"] };
	}
	else if (type_ == SPOTLIGHT) {
		param_ = Spot{ color, intensity, {0,0,0}, {0,0,1}, j["distance"], j["decay"], j["cosAngle"] };
	}
	else if (type_ == RECTLIGHT) {
		param_ = Rect{ color, intensity, {0,0,0}, {0,0,1}, { j["size"][0], j["size"][1] }, {1,0,0}, {0,1,0}, j["decay"] };
	}
}

#pragma region アクセッサ
Vector4 LightComponent::GetColor() const {
	if(auto* p = std::get_if<Directional>(&param_)) return p->color;
	if(auto* p = std::get_if<Point>(&param_))		return p->color;
	if(auto* p = std::get_if<Spot>(&param_))		return p->color;
	if(auto* p = std::get_if<Rect>(&param_))		return p->color;

	return { 1.0f, 1.0f, 1.0f, 1.0f };
}

void LightComponent::SetColor(const Vector4& color) {
	if(auto* p = std::get_if<Directional>(&param_)) { p->color = color; return; }
	if(auto* p = std::get_if<Point>(&param_)) { p->color = color; return; }
	if(auto* p = std::get_if<Spot>(&param_)) { p->color = color; return; }
	if(auto* p = std::get_if<Rect>(&param_)) { p->color = color; return; }
}

float LightComponent::GetIntensity() const {
	if(auto* p = std::get_if<Directional>(&param_)) return p->intensity;
	if(auto* p = std::get_if<Point>(&param_))		return p->intensity;
	if(auto* p = std::get_if<Spot>(&param_))		return p->intensity;
	if(auto* p = std::get_if<Rect>(&param_))		return p->intensity;

	return 0.0f;
}

void LightComponent::SetIntensity(const float& intensity) {
	if(auto* p = std::get_if<Directional>(&param_)) { p->intensity = intensity; return; }
	if(auto* p = std::get_if<Point>(&param_)) { p->intensity = intensity; return; }
	if(auto* p = std::get_if<Spot>(&param_)) { p->intensity = intensity; return; }
	if(auto* p = std::get_if<Rect>(&param_)) { p->intensity = intensity; return; }
}

Vector3 LightComponent::GetPosition() const {
	if(auto* p = std::get_if<Point>(&param_)) return p->position;
	if(auto* p = std::get_if<Spot>(&param_)) return p->position;
	if(auto* p = std::get_if<Rect>(&param_)) return p->position;

	return { 0.0f, 0.0f, 0.0f };
}

void LightComponent::SetPosition(const Vector3& position) {
	if(auto* p = std::get_if<Point>(&param_)) { p->position = position; return; }
	if(auto* p = std::get_if<Spot>(&param_)) { p->position = position; return; }
	if(auto* p = std::get_if<Rect>(&param_)) { p->position = position; return; }
}

Vector3 LightComponent::GetDirection() const {
	if(auto* p = std::get_if<Directional>(&param_)) return p->direction;
	if(auto* p = std::get_if<Spot>(&param_)) return p->direction;
	if(auto* p = std::get_if<Rect>(&param_)) return p->direction;

	return { 0.0f, 0.0f, 0.0f };
}

void LightComponent::SetDirection(const Vector3& direction) {
	if(auto* p = std::get_if<Directional>(&param_)) { p->direction = direction; return; }
	if(auto* p = std::get_if<Spot>(&param_)) { p->direction = direction; return; }
	if(auto* p = std::get_if<Rect>(&param_)) { p->direction = direction; return; }
}

float LightComponent::GetRadius() const {
	if(auto* p = std::get_if<Point>(&param_)) return p->radius;

	return 0.0f;
}

void LightComponent::SetRadius(const float& radius) {
	if(auto* p = std::get_if<Point>(&param_)) { p->radius = radius; return; }
}

float LightComponent::GetDecay() const {
	if(auto* p = std::get_if<Point>(&param_)) return p->decay;
	if(auto* p = std::get_if<Spot>(&param_)) return p->decay;
	if(auto* p = std::get_if<Rect>(&param_)) return p->decay;

	return 0.0f;
}

void LightComponent::SetDecay(const float& decay) {
	if(auto* p = std::get_if<Point>(&param_)) { p->decay = decay; return; }
	if(auto* p = std::get_if<Spot>(&param_)) { p->decay = decay; return; }
	if(auto* p = std::get_if<Rect>(&param_)) { p->decay = decay; return; }
}

float LightComponent::GetDistance() const {
	if(auto* p = std::get_if<Spot>(&param_)) return p->distance;

	return 0.0f;
}

void LightComponent::SetDistance(const float& distance) {
	if(auto* p = std::get_if<Spot>(&param_)) { p->distance = distance; return; }
}

float LightComponent::GetCosAngle() const {
	if(auto* p = std::get_if<Spot>(&param_)) return p->cosAngle;

	return 0.0f;
}

void LightComponent::SetCosAngle(const float& cosAngle) {
	if(auto* p = std::get_if<Spot>(&param_)) { p->cosAngle = cosAngle; return; }
}

Vector2 LightComponent::GetSize() const {
	if(auto* p = std::get_if<Rect>(&param_)) return p->size;

	return { 0.0f, 0.0f };
}

void LightComponent::SetSize(const Vector2& size) {
	if(auto* p = std::get_if<Rect>(&param_)) { p->size = size; return; }
}

const Directional* LightComponent::GetDirectionalParam() const {
	return std::get_if<Directional>(&param_);
}

const Point* LightComponent::GetPointParam() const {
	return std::get_if<Point>(&param_);
}

const Spot* LightComponent::GetSpotParam() const {
	return std::get_if<Spot>(&param_);
}

const Rect* LightComponent::GetRectParam() const {
	return std::get_if<Rect>(&param_);
}
#pragma endregion