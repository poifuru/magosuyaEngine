#include "PCH.h"
#include "GpuEmitterComponent.h"
#include "GameObject.h"
#include "BaseScene.h"
#include "Renderer.h"
#include "ParticleEffectData.h"
#include "Time.h"

void GpuEmitterComponent::Initialize() {
	isPlaying_ = false;
	timer_.accumulator = 0.0f;
	if (playOnStart_) {
		Play();
	}
}

void GpuEmitterComponent::Update() {
	if (!isPlaying_) { return; }

	GpuParticleSystem* system = GetSystem();
	if (!system) { return; }
	ParticleEffectData* data = system->FindEffect(effectName_);
	if (!data || !data->loop) { return; }

	// 「1秒あたり rate 個」をこのフレームの個数に直して出す
	uint32_t count = timer_.Step(data->rate, Time::GetDeltaTime());
	system->Emit(effectName_, GetEmitPosition(), count);
}

void GpuEmitterComponent::Play() {
	GpuParticleSystem* system = GetSystem();
	if (!system) { return; }
	ParticleEffectData* data = system->FindEffect(effectName_);
	if (!data) { return; }   // その名前のエフェクトが無い（LoadAll より前に呼ばれた、名前の打ち間違いなど）

	if (data->loop) {
		isPlaying_ = true;   // Update で出し続ける
	}
	else {
		system->Burst(effectName_, GetEmitPosition());   // 一発物はここで終わり
	}
}

void GpuEmitterComponent::Stop() {
	isPlaying_ = false;
}

GpuParticleSystem* GpuEmitterComponent::GetSystem() const {
	if (!gameObject_) { return nullptr; }
	SceneContext* context = gameObject_->GetContext();
	if (!context || !context->renderer) { return nullptr; }
	return context->renderer->GetParticleSystem();
}

Vector3 GpuEmitterComponent::GetEmitPosition() const {
	if (!gameObject_) { return offset_; }
	const Vector3& p = gameObject_->GetTransform().translate;
	return { p.x + offset_.x, p.y + offset_.y, p.z + offset_.z };
}

void GpuEmitterComponent::ImGui() {
#ifdef USEIMGUI
	GpuParticleSystem* system = GetSystem();

	// エフェクト名：読み込んだエフェクトの中から選ぶ
	if (ImGui::BeginCombo("エフェクト", effectName_.c_str())) {
		if (system) {
			for (const std::string& name : system->GetEffectNames()) {
				if (ImGui::Selectable(name.c_str(), name == effectName_)) {
					effectName_ = name;
					timer_.accumulator = 0.0f;
				}
			}
		}
		ImGui::EndCombo();
	}
	if (system && !system->FindEffect(effectName_)) {
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "このエフェクトは見つかりません");
	}

	ImGui::Checkbox("開始時に再生", &playOnStart_);
	ImGui::DragFloat3("位置のずらし", &offset_.x, 0.05f);

	if (ImGui::Button("再生")) { Play(); }
	ImGui::SameLine();
	if (ImGui::Button("停止")) { Stop(); }
	ImGui::SameLine();
	ImGui::TextUnformatted(isPlaying_ ? "再生中" : "停止中");
#endif
}

void GpuEmitterComponent::Serialize(json& j) const {
	j["type"] = "GpuEmitterComponent";   // ComponentRegistry の REGISTER_COMPONENT の型名と同じにする
	j["effectName"] = effectName_;
	j["playOnStart"] = playOnStart_;
	j["offset"] = { offset_.x, offset_.y, offset_.z };
}

void GpuEmitterComponent::Deserialize(const json& j) {
	if (j.contains("effectName")) effectName_ = j["effectName"].get<std::string>();
	if (j.contains("playOnStart")) playOnStart_ = j["playOnStart"];
	if (j.contains("offset") && j["offset"].is_array() && j["offset"].size() >= 3) {
		offset_ = { j["offset"][0], j["offset"][1], j["offset"][2] };
	}
}
