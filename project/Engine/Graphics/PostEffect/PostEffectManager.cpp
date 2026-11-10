#include "PCH.h"
#include "PostEffectManager.h"
#include "PostEffectType.h"

void PostEffectManager::Initialize(ID3D12Device* device) {
	// ポストエフェクトをすべて初期化
	// 最初はすべて非アクティブ状態にしておく
	effects_[static_cast<size_t>(PostEffectType::Outline)] = std::make_unique<Outline>();
	effects_[static_cast<size_t>(PostEffectType::ColorGrading)] = std::make_unique<ColorGrading>();
	effects_[static_cast<size_t>(PostEffectType::Fog)] = std::make_unique<Fog>();
	effects_[static_cast<size_t>(PostEffectType::Vignette)] = std::make_unique<Vignette>();
	effects_[static_cast<size_t>(PostEffectType::RadialBlur)] = std::make_unique<RadialBlur>();
	effects_[static_cast<size_t>(PostEffectType::Dissolve)] = std::make_unique<Dissolve>();
	effects_[static_cast<size_t>(PostEffectType::RandomNoise)] = std::make_unique<RandomNoise>();

	for (size_t i = 0; i < static_cast<size_t>(PostEffectType::Count); ++i) {
		if (effects_[i] != nullptr) {
			effects_[i]->Initialize(device);
			effects_[i]->SetIsActive(false); // 初期状態はOFF
		}
	}
}

void PostEffectManager::SetEffectActive(PostEffectType type, bool flag) {
	size_t index = static_cast<size_t>(type);
	if (effects_[index] != nullptr) {
		effects_[index]->SetIsActive(flag);
	}
}

void PostEffectManager::ClearEffects() {
	// インスタンスは消さずに、フラグをすべて false にリセットするだけ
	for (size_t i = 0; i < static_cast<size_t>(PostEffectType::Count); ++i) {
		if (effects_[i] != nullptr) {
			effects_[i]->SetIsActive(false);
		}
	}
}

void PostEffectManager::ImGui() {
#ifdef USEIMGUI
	// ポストエフェクト全体の管理ウィンドウを開く
	ImGui::Begin("ポストエフェクト");

	// 全エフェクトの ON/OFF チェックボックスを一覧表示する
	ImGui::Text("オン・オフ");

	for (size_t i = 0; i < static_cast<size_t>(PostEffectType::Count); ++i) {
		if (effects_[i] == nullptr) continue;

		// 列挙型に応じて表示する名前を決定する
		const char* effectName = effects_[i]->GetDisplayName();

		// 現在のフラグ状態を取得
		bool isActive = effects_[i]->GetIsActive();

		// チェックボックスを表示（クリックされたら自動で isActive が書き換わる）
		if (ImGui::Checkbox(effectName, &isActive)) {
			effects_[i]->SetIsActive(isActive);
		}
	}

	for(size_t i = 0; i < static_cast<size_t>(PostEffectType::Count); ++i) {
		ImGui::Separator();
		ImGui::Text("パラメータ");

		std::string headerName = std::format("{}の設定", effects_[i]->GetDisplayName());

		if(ImGui::CollapsingHeader(headerName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
			effects_[i]->ImGui();
		}
	}

	ImGui::End(); // ポストエフェクト管理ウィンドウを閉じる
#endif
}
