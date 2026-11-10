#include "PCH.h"
#include "LightManager.h"
#include "LightComponent.h"
#include "ColorUtils.h"

void LightManager::Initialize(ID3D12Device* device) {
	assert(device != nullptr);
	lightBuffer_.Initialize(device);
}

void LightManager::Update() {
	// 環境光の色を Linear 化して intensity を掛け合わせて構造体にセット
	Vector3 linearColor = ColorUtils::ToLinear(ambientColor_) * ambientIntensity_;
	lightCPUData_.ambientColor = Vector4(linearColor.x, linearColor.y, linearColor.z, 1.0f);

	// コンポーネントから集約されたデータをそのままGPUへ転送するだけ
	lightBuffer_.Update(lightCPUData_);
}

void LightManager::ImGui() {
#ifdef USEIMGUI
	ImGui::Begin("ライト管理");
	// 環境光 (Ambient Light)
	if (ImGui::CollapsingHeader("環境光", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::ColorEdit3("環境光の色", &ambientColor_.x);
		ImGui::DragFloat("環境光の強さ", &ambientIntensity_, 0.01f, 0.0f, 5.0f);
	}
	ImGui::Separator();

	// 有効ライト数のサマリー表示
	ImGui::Text("使っているライトの数:");
	ImGui::Text("  平行光源      : %d / %d", lightCPUData_.count.dirLight, MaxCount);
	ImGui::Text("  点光源        : %d / %d", lightCPUData_.count.pointLight, MaxCount);
	ImGui::Text("  スポットライト: %d / %d", lightCPUData_.count.spotLight, MaxCount);
	ImGui::Text("  面光源        : %d / %d", lightCPUData_.count.rectLight, MaxCount);

	ImGui::Separator();
	
	ImGui::End();
#endif
}

void LightManager::ClearLights() {
	// CPU側の一時データ（GPU送信用）のカウントを全てリセットする
	lightCPUData_.count.dirLight = 0;
	lightCPUData_.count.pointLight = 0;
	lightCPUData_.count.spotLight = 0;
	lightCPUData_.count.rectLight = 0;
}

void LightManager::Register(const LightComponent* light) {
	if(!light) return;

	// variantの実際の型に合ったオペレータが呼ばれる
	std::visit(RegisterVisitor{ this }, light->GetParam());
}

void LightManager::RegisterDirectionalLight(const Vector4& color, const Vector3& direction, float intensity) {
	int32_t index = lightCPUData_.count.dirLight;
	if (index >= MaxCount) return;
	lightCPUData_.dirLights[index].color = ColorUtils::ToLinear(color);
	lightCPUData_.dirLights[index].direction = direction;
	lightCPUData_.dirLights[index].intensity = intensity;
	lightCPUData_.count.dirLight++;
}

void LightManager::RegisterPointLight(const Vector4& color, const Vector3& position, float intensity, 
									  float radius, float decay
) {
	int32_t index = lightCPUData_.count.pointLight;
	if (index >= MaxCount) return;
	lightCPUData_.pointLights[index].color = ColorUtils::ToLinear(color);
	lightCPUData_.pointLights[index].position = position;
	lightCPUData_.pointLights[index].intensity = intensity;
	lightCPUData_.pointLights[index].radius = radius;
	lightCPUData_.pointLights[index].decay = decay;
	lightCPUData_.count.pointLight++;
}

void LightManager::RegisterSpotLight(const Vector4& color, const Vector3& position, float intensity, 
									 const Vector3& direction, float distance, float decay, float cosAngle
) {
	int32_t index = lightCPUData_.count.spotLight;
	if (index >= MaxCount) return;
	lightCPUData_.spotLights[index].color = ColorUtils::ToLinear(color);
	lightCPUData_.spotLights[index].position = position;
	lightCPUData_.spotLights[index].intensity = intensity;
	lightCPUData_.spotLights[index].direction = direction;
	lightCPUData_.spotLights[index].distance = distance;
	lightCPUData_.spotLights[index].decay = decay;
	lightCPUData_.spotLights[index].cosAngle = cosAngle;
	lightCPUData_.count.spotLight++;
}

void LightManager::RegisterRectLight(const Vector4& color, const Vector3& position, float intensity, 
									 const Vector3& direction, const Vector2& size, 
									 const Vector3& right, const Vector3& up, float decay
) {
	int32_t index = lightCPUData_.count.rectLight;
	if (index >= MaxCount) return;
	lightCPUData_.rectLights[index].color = ColorUtils::ToLinear(color);
	lightCPUData_.rectLights[index].position = position;
	lightCPUData_.rectLights[index].intensity = intensity;
	lightCPUData_.rectLights[index].direction = direction;
	lightCPUData_.rectLights[index].size = size;
	lightCPUData_.rectLights[index].right = right;
	lightCPUData_.rectLights[index].up = up;
	lightCPUData_.rectLights[index].decay = decay;
	lightCPUData_.count.rectLight++;
}

void LightManager::RegisterVisitor::operator()(const Directional& p) const {
	manager->RegisterDirectionalLight(p.color, p.direction, p.intensity);
}

void LightManager::RegisterVisitor::operator()(const Point& p) const {
	manager->RegisterPointLight(p.color, p.position, p.intensity, p.radius, p.decay);
}

void LightManager::RegisterVisitor::operator()(const Spot& p) const {
	manager->RegisterSpotLight(p.color, p.position, p.intensity, p.direction, p.distance, p.decay, p.cosAngle);
}

void LightManager::RegisterVisitor::operator()(const Rect& p) const {
	manager->RegisterRectLight(p.color, p.position, p.intensity, p.direction, p.size, p.right, p.up, p.decay);
}