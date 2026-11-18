#include "PCH.h"
#include "ParticleEditorWindow.h"
#include "BaseScene.h"
#include "Renderer.h"
#include "GpuParticleSystem.h"
#include "GpuParticlePool.h"
#include "ParticleEffectData.h"
#include "Time.h"

namespace {
	// Combo の表示名（並びは enum / 番号と同じにする）
	const char* kShapeNames[] = { "点", "球", "箱", "円錐" };
	const char* kBlendNames[] = { "不透明", "アルファ", "加算", "減算", "乗算", "スクリーン" };   // BlendModeType と同じ順

	// 範囲（x:最小 y:最大）が逆にならないようにする
	void FixRange(Vector2& range) {
		if (range.x > range.y) { range.y = range.x; }
	}
}

ParticleEditorWindow::ParticleEditorWindow()
	: IEditorWindow("パーティクルエディタ", false) {   // 最初は閉じておく（メニューから開く）
}

void ParticleEditorWindow::UpdateAndDraw(const EditorContext& context) {
	if (!isOpen_) { return; }

	ImGui::SetNextWindowSize(ImVec2(820.0f, 560.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin(name_.c_str(), &isOpen_)) {
		// System は SceneContext の renderer から取る
		GpuParticleSystem* system = nullptr;
		if (context.sceneContext && context.sceneContext->renderer) {
			system = context.sceneContext->renderer->GetParticleSystem();
		}

		if (!system) {
			ImGui::TextDisabled("パーティクルシステムがありません");
		}
		else {
			// 左：一覧（幅 200px の子ウィンドウ）
			ImGui::BeginChild("##list", ImVec2(200.0f, 0.0f), true);
			DrawEffectList(*system);
			ImGui::EndChild();

			ImGui::SameLine();

			// 右：プレビューとパラメータ
			ImGui::BeginChild("##params");
			ParticleEffectData* data = system->FindEffect(selectedName_);
			if (!data) {
				ImGui::TextDisabled("左の一覧からエフェクトを選んでください");
			}
			else {
				DrawPreview(*system);
				if (DrawParameters(*system, *data)) {
					system->ApplyEffect(selectedName_);   // 変わったフレームだけ反映
					unsavedNames_.insert(selectedName_);
				}
			}
			ImGui::EndChild();
		}
	}
	ImGui::End();   // Begin の戻り値が false（たたまれている）でも End は必ず呼ぶ
}

void ParticleEditorWindow::DrawEffectList(GpuParticleSystem& system) {
	// ---- 一覧 ----
	ImGui::TextDisabled("エフェクト");
	// GetEffectNames() はコピーを返すので、ループ中に一覧が増えても壊れない
	for (const std::string& name : system.GetEffectNames()) {
		// 未保存なら名前の後ろに * を付ける。ID は ## の後ろの名前のままにして、* が付いても選択が外れないようにする
		std::string label = name;
		if (unsavedNames_.count(name) > 0) { label += " *"; }
		label += "##" + name;

		if (ImGui::Selectable(label.c_str(), name == selectedName_)) {
			selectedName_ = name;
			message_.clear();
		}
	}

	ImGui::Separator();

	// ---- 新規作成 ----
	ImGui::TextDisabled("新規作成（半角英数字と _）");
	ImGui::SetNextItemWidth(-1.0f);   // 子ウィンドウの幅いっぱい
	ImGui::InputText("##newName", newNameBuffer_, sizeof(newNameBuffer_));
	if (ImGui::Button("新規作成") && newNameBuffer_[0] != '\0') {
		if (system.CreateEffect(newNameBuffer_)) {
			selectedName_ = newNameBuffer_;
			message_ = selectedName_ + ".json を作りました";
			newNameBuffer_[0] = '\0';
		}
		else {
			message_ = "作れませんでした（同じ名前があるか、使えない文字が入っています）";
		}
	}

	ImGui::Separator();

	// ---- 保存 ----
	ImGui::BeginDisabled(selectedName_.empty());
	if (ImGui::Button("保存")) {
		if (system.SaveEffect(selectedName_)) {
			unsavedNames_.erase(selectedName_);
			message_ = selectedName_ + ".json に保存しました";
		}
		else {
			message_ = "保存できませんでした";
		}
	}
	ImGui::EndDisabled();

	if (!message_.empty()) {
		ImGui::TextWrapped("%s", message_.c_str());
	}
}

void ParticleEditorWindow::DrawPreview(GpuParticleSystem& system) {
	ImGui::Text("編集中: %s", selectedName_.c_str());

	// バースト再生と、出し続けるプレビューを出す位置
	ImGui::DragFloat3("プレビュー位置", &previewPosition_.x, 0.05f);

	// 出し続けるエフェクトは、エディタが毎フレーム要求を出す（Pool は自分では出さないため）
	// ウィンドウを閉じたり別のエフェクトを選ぶと要求が止まるので、止める処理はいらない
	ImGui::Checkbox("その場で出し続ける", &previewLoop_);
	ParticleEffectData* data = system.FindEffect(selectedName_);
	if (previewLoop_ && data && data->loop) {
		uint32_t count = previewTimer_.Step(data->rate, Time::GetDeltaTime());
		system.Emit(selectedName_, previewPosition_, count);
	}
	ImGui::SameLine();
	if (ImGui::Button("バースト再生")) {
		system.Burst(selectedName_, previewPosition_);
	}

	// 生きている粒の数 = 最大数 - 空きの数（頂上 + 1）。リードバックなので1フレーム遅れ
	GpuParticlePool* pool = system.FindPool(selectedName_);
	if (pool) {
		int32_t top = pool->GetFreeListTop();
		int32_t maxCount = static_cast<int32_t>(pool->GetMaxCount());
		int32_t alive = maxCount - (top + 1);
		ImGui::SameLine();
		ImGui::Text("生きている粒: %d / %d", alive, maxCount);
	}
	ImGui::Separator();
}

bool ParticleEditorWindow::DrawParameters(GpuParticleSystem& system, ParticleEffectData& d) {
	// ImGui の入力関数は「値が変わったフレームだけ true」を返すので、まとめて返す
	bool changed = false;

	// ---- 放出 ----
	if (ImGui::CollapsingHeader("放出", ImGuiTreeNodeFlags_DefaultOpen)) {
		changed |= ImGui::Checkbox("出し続ける", &d.loop);
		if (d.loop) {
			changed |= ImGui::DragFloat("1秒あたりの数", &d.rate, 1.0f, 0.0f, 10000.0f);
		}

		// uint32_t は DragInt に直接渡せないので int を挟む
		int burst = static_cast<int>(d.burstCount);
		if (ImGui::DragInt("バーストの数", &burst, 1.0f, 0, 100000)) {
			d.burstCount = static_cast<uint32_t>(burst);
			changed = true;
		}

		// 最大数は Pool の作り直しになるので、入力を確定したとき（Enter・フォーカスが外れたとき）だけ反映する
		int maxCount = static_cast<int>(d.maxCount);
		ImGui::InputInt("最大数", &maxCount, 256, 1024);
		if (ImGui::IsItemDeactivatedAfterEdit() && maxCount >= 64) {
			d.maxCount = static_cast<uint32_t>(maxCount);
			changed = true;
		}
	}

	// ---- 形 ----
	if (ImGui::CollapsingHeader("形", ImGuiTreeNodeFlags_DefaultOpen)) {
		int shape = static_cast<int>(d.shape);
		if (ImGui::Combo("形状", &shape, kShapeNames, IM_ARRAYSIZE(kShapeNames))) {
			d.shape = static_cast<uint32_t>(shape);
			changed = true;
		}
		changed |= ImGui::DragFloat("半径", &d.radius, 0.01f, 0.0f, 100.0f);
		changed |= ImGui::DragFloat3("箱の大きさ", &d.boxSize.x, 0.01f, 0.0f, 100.0f);
		changed |= ImGui::DragFloat3("向き", &d.direction.x, 0.01f, -1.0f, 1.0f);
		changed |= ImGui::DragFloat("広がり角度", &d.spreadAngle, 0.5f, 0.0f, 180.0f, "%.1f 度");
	}

	// ---- 寿命と速さ ----
	if (ImGui::CollapsingHeader("寿命と速さ", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (ImGui::DragFloat2("寿命（最小/最大）", &d.lifeTime.x, 0.01f, 0.01f, 60.0f, "%.2f 秒")) {
			FixRange(d.lifeTime);
			changed = true;
		}
		if (ImGui::DragFloat2("速さ（最小/最大）", &d.speed.x, 0.05f, 0.0f, 200.0f)) {
			FixRange(d.speed);
			changed = true;
		}
	}

	// ---- 力 ----
	if (ImGui::CollapsingHeader("力", ImGuiTreeNodeFlags_DefaultOpen)) {
		changed |= ImGui::DragFloat3("加速度", &d.acceleration.x, 0.05f);
		changed |= ImGui::DragFloat("空気抵抗", &d.drag, 0.01f, 0.0f, 20.0f);
	}

	// ---- 見た目 ----
	if (ImGui::CollapsingHeader("見た目", ImGuiTreeNodeFlags_DefaultOpen)) {
		// 同じラベルは同じ部品だと思われるので、## の後ろで区別する（## 以降は画面に出ない）
		changed |= ImGui::ColorEdit4("開始色##start", &d.startColor.x);
		changed |= ImGui::ColorEdit4("終了色##end", &d.endColor.x);
		if (ImGui::DragFloat2("開始サイズ（最小/最大）", &d.startSize.x, 0.01f, 0.0f, 50.0f)) {
			FixRange(d.startSize);
			changed = true;
		}
		changed |= ImGui::DragFloat("終わりの倍率", &d.endSize, 0.01f, 0.0f, 10.0f);

		int blend = static_cast<int>(d.blend);
		if (ImGui::Combo("ブレンド", &blend, kBlendNames, IM_ARRAYSIZE(kBlendNames))) {
			d.blend = static_cast<MyEngine::Rendering::BlendModeType>(blend);
			changed = true;
		}

		// テクスチャ：LoadAll で先読みした中から選ぶ（ゲーム中は新しく読み込めないため）
		if (ImGui::BeginCombo("テクスチャ", d.texture.c_str())) {
			for (const std::string& path : system.GetTexturePaths()) {
				if (ImGui::Selectable(path.c_str(), path == d.texture)) {
					d.texture = path;
					changed = true;
				}
			}
			ImGui::EndCombo();   // BeginCombo が true のときだけ呼ぶ
		}
	}

	return changed;
}
