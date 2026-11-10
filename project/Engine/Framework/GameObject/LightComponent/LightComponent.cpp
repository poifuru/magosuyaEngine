#include "PCH.h"
#include "LightComponent.h"
#include "GameObject.h"
#include "MathFunction.h"


namespace {
#pragma region ライトの種類ごとの値をJSONに書き出す
	struct LightParamWriter {
		json& j;

		void operator()(const Directional& p) const{
			j["color"] = { p.color.x, p.color.y, p.color.z, p.color.w };
			j["intensity"] = p.intensity;
		}

		void operator()(const Point& p) const {
			j["color"] = { p.color.x, p.color.y, p.color.z, p.color.w };
			j["intensity"] = p.intensity;
			j["radius"] = p.radius;
			j["decay"] = p.decay;
		}

		void operator()(const Spot& p) const {
			j["color"] = { p.color.x, p.color.y, p.color.z, p.color.w };
			j["intensity"] = p.intensity;
			j["distance"] = p.distance;
			j["decay"] = p.decay;
			j["cosAngle"] = p.cosAngle;
		}

		void operator()(const Rect& p) const{
			j["color"] = { p.color.x, p.color.y, p.color.z, p.color.w };
			j["intensity"] = p.intensity;
			j["size"] = { p.size.x, p.size.y };
			j["decay"] = p.decay;
		}
	};
#pragma endregion

#pragma region ライトの値をJSONから読み取る
	struct LightParamReader {
		const json& j;

		void operator()(Directional& p) const{
			// 値が1つのものはj.valueを使って安全に読み込み
			p.intensity = j.value("intensity", p.intensity);

			// 他の値は配列を指定して読み込み
			if(j.contains("color")) {
				p.color = { j["color"][0], j["color"][1],j["color"][2],j["color"][3] };
			}
		}

		void operator()(Point& p) const {
			p.intensity = j.value("intensity", p.intensity);
			p.radius = j.value("radius", p.radius);
			p.decay = j.value("decay", p.decay);

			if(j.contains("color")) {
				p.color = { j["color"][0], j["color"][1],j["color"][2],j["color"][3] };
			}
		}

		void operator()(Spot& p) const {
			p.intensity = j.value("intensity", p.intensity);
			p.distance = j.value("distance", p.distance);
			p.decay = j.value("decay", p.decay);
			p.cosAngle = j.value("cosAngle", p.cosAngle);

			if(j.contains("color")) {
				p.color = { j["color"][0], j["color"][1],j["color"][2],j["color"][3] };
			}
		}

		void operator()(Rect& p) const {
			p.intensity = j.value("intensity", p.intensity);
			p.decay = j.value("decay", p.decay);

			if(j.contains("color")) {
				p.color = { j["color"][0], j["color"][1],j["color"][2],j["color"][3] };
			}
			if(j.contains("size")) {
				p.size = { j["size"][0], j["size"][1] };
			}
		}
	};
#pragma endregion

#pragma region エディタで編集する
	struct LightParamEditor {
		void operator()(Directional& p) const {
			ImGui::ColorEdit4("色", &p.color.x);
			ImGui::DragFloat("強さ", &p.intensity, 0.05f, 0.0f, 100.0f);
		}

		void operator()(Point& p) const{
			ImGui::ColorEdit4("色", &p.color.x);
			ImGui::DragFloat("強さ", &p.intensity, 0.05f, 0.0f, 100.0f);
			ImGui::DragFloat("半径", &p.radius, 0.1f, 0.0f, 100.0f);
			ImGui::DragFloat("減衰", &p.decay, 0.05f, 0.0f, 10.0f);
		}

		void operator()(Spot& p) const {
			ImGui::ColorEdit4("色", &p.color.x);
			ImGui::DragFloat("強さ", &p.intensity, 0.05f, 0.0f, 100.0f);
			ImGui::DragFloat("距離", &p.distance, 0.1f, 0.0f, 100.0f);
			ImGui::DragFloat("減衰", &p.decay, 0.05f, 0.0f, 10.0f);
			// コサインだと角度の感覚が掴みづらいので、度数法(0~180度)に変換して編集できるようにする
			float angle = acosf(p.cosAngle) * (180.0f / 3.14159265f) * 2.0f;
			if (ImGui::DragFloat("角度", &angle, 0.5f, 0.0f, 180.0f)) {
				p.cosAngle = cosf((angle * 0.5f) * (3.14159265f / 180.0f));
			}
		}

		void operator()(Rect& p) const {
			ImGui::ColorEdit4("色", &p.color.x);
			ImGui::DragFloat("強さ", &p.intensity, 0.05f, 0.0f, 100.0f);
			ImGui::DragFloat2("大きさ", &p.size.x, 0.05f, 0.0f, 50.0f);
			ImGui::DragFloat("減衰", &p.decay, 0.05f, 0.0f, 10.0f);
		}
	};
#pragma endregion

#pragma region 更新処理
	struct LightTransformSync {
		const Vector3& position;
		const Matrix4x4& rotMatrix;

		void operator()(Directional& p) const {
			p.direction = Math::Transform({ 0.0f, -1.0f, 0.0f }, rotMatrix);
		}

		void operator()(Point& p) const {
			p.position = position;
		}

		void operator()(Spot& p) const {
			p.position = position;
			p.direction = Math::Transform({ 0.0f, 0.0f, 1.0f }, rotMatrix);
		}

		void operator()(Rect& p) const {
			p.position = position;
			p.direction = Math::Transform({ 0.0f, 0.0f, 1.0f }, rotMatrix);
			p.right = Math::Transform({ 1.0f, 0.0f, 0.0f }, rotMatrix);
			p.up = Math::Transform({ 0.0f, 1.0f, 0.0f }, rotMatrix);
		}
	};
#pragma endregion
}

LightParam LightComponent::MakeDefaultParam(size_t index) {
	const Vector4 white = { 1.0f, 1.0f, 1.0f, 1.0f };
	switch (index) {
	case POINTLIGHT:
		return Point{ white, 1.0f, { 0.0f, 0.0f, 0.0f }, 10.0f, 1.0f };
	case SPOTLIGHT:
		return Spot{ white, 1.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, 10.0f, 1.0f, 0.5f };
	case RECTLIGHT:
		return Rect{ white, 1.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f },
					 { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 1.0f };
	case DIRECTIONALLIGHT:
	default:
		return Directional{ white, 1.0f, { 0.0f, -1.0f, 0.0f } };
	}
}

void LightComponent::Initialize() {
	if (isInitialized_) return;
	isInitialized_ = true;

	// デフォルトのDirectionalLightで初期化しておく
	param_ = MakeDefaultParam(DIRECTIONALLIGHT);
}

void LightComponent::Update() {
	GameObject* owner = GetGameObject();
	if(!owner) return;

	// Transformの取得
	auto& transform = owner->GetTransform();

	// 各軸の回転から Quaternion を作成して合成する
	Quaternion qX = Math::MakeRotateAxisAngleQuaternion({1.0f, 0.0f, 0.0f}, transform.rotate.x);
	Quaternion qY = Math::MakeRotateAxisAngleQuaternion({0.0f, 1.0f, 0.0f}, transform.rotate.y);
	Quaternion qZ = Math::MakeRotateAxisAngleQuaternion({0.0f, 0.0f, 1.0f}, transform.rotate.z);
	Quaternion q = Math::Multiply(Math::Multiply(qX, qY), qZ);
	// Quaternion から回転行列を作る
	Matrix4x4 rotMat = Math::MakeRotateMatrix(q);

	// パラメータを必要としているライトに同期させる
	std::visit(LightTransformSync{ transform.translate, rotMat }, param_);
}

void LightComponent::ImGui() {
	// ライトタイプの切り替え
	const char* types[] = { "平行光源", "点光源", "スポットライト", "面光源" };
	int currentType = static_cast<int>(param_.index());
	if(ImGui::Combo("種類", &currentType, types, IM_ARRAYSIZE(types))) {
		// タイプが切り替わったら variant の中身をその種類の初期値で作り直す
		param_ = MakeDefaultParam(static_cast<size_t>(currentType));
	}

	ImGui::Separator();

	// 個別パラメータの編集
	std::visit(LightParamEditor{}, param_);
}

void LightComponent::Serialize(json& j) const {
	j["type"] = "LightComponent";
	j["lightType"] = static_cast<int>(param_.index());

	// オペレータに登録した処理を使う
	std::visit(LightParamWriter{ j }, param_);
}

void LightComponent::Deserialize(const json& j) {
	isInitialized_ = true;

	// まず種類ごとの初期値で作り、JSON にある項目だけ上書きする
	// （項目が欠けていても初期値が残るので、古いシーンファイルでも落ちない）
	size_t index = j.value("lightType", static_cast<int>(DIRECTIONALLIGHT));
	if (index >= std::variant_size_v<LightParam>) index = DIRECTIONALLIGHT;
	param_ = MakeDefaultParam(index);

	// オペレータに登録した処理を使う
	std::visit(LightParamReader{ j }, param_);
}

#pragma region アクセッサ用の構造体
namespace {
	struct ColorGetter {
		template <class T>
		Vector4 operator()(const T& p) const { return p.color; }
	};

	struct ColorSetter {
		const Vector4& color;

		template <class T>
		void operator()(T& p) const { p.color = color; }
	};

	struct IntensityGetter {
		template <class T>
		float operator()(const T& p) const { return p.intensity; }
	};

	struct IntensitySetter {
		float intensity;

		template <class T>
		void operator()(T& p) const { p.intensity = intensity; }
	};
}
#pragma endregion

#pragma region アクセッサ
// 全種類が持っている値
Vector4 LightComponent::GetColor() const {
	return std::visit(ColorGetter{}, param_);
}

void LightComponent::SetColor(const Vector4& color) {
	std::visit(ColorSetter{ color }, param_);
}

float LightComponent::GetIntensity() const {
	return std::visit(IntensityGetter{}, param_);
}

void LightComponent::SetIntensity(const float& intensity) {
	std::visit(IntensitySetter{ intensity }, param_);
}

// 1種類だけが持っている値
float LightComponent::GetRadius() const {
	if (auto* p = std::get_if<Point>(&param_)) return p->radius;
	return 0.0f;
}

void LightComponent::SetRadius(const float& radius) {
	if (auto* p = std::get_if<Point>(&param_)) p->radius = radius;
}

float LightComponent::GetDistance() const {
	if (auto* p = std::get_if<Spot>(&param_)) return p->distance;
	return 0.0f;
}

void LightComponent::SetDistance(const float& distance) {
	if (auto* p = std::get_if<Spot>(&param_)) p->distance = distance;
}

float LightComponent::GetCosAngle() const {
	if (auto* p = std::get_if<Spot>(&param_)) return p->cosAngle;
	return 0.0f;
}

void LightComponent::SetCosAngle(const float& cosAngle) {
	if (auto* p = std::get_if<Spot>(&param_)) p->cosAngle = cosAngle;
}

Vector2 LightComponent::GetSize() const {
	if (auto* p = std::get_if<Rect>(&param_)) return p->size;
	return { 0.0f, 0.0f };
}

void LightComponent::SetSize(const Vector2& size) {
	if (auto* p = std::get_if<Rect>(&param_)) p->size = size;
}
#pragma endregion
