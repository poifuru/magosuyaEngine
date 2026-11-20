#include "PCH.h"
#include "ParticleEditorWindow.h"
#include "BaseScene.h"
#include "Renderer.h"
#include "GpuParticleSystem.h"
#include "GpuParticlePool.h"
#include "ParticleEffectData.h"
#include "Time.h"
#include "CameraOrganizer.h"
#include "MathFunction.h"
#include "Renderer.h"
#include "LineRenderer.h"

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
		DrawContents(context);
	}
	ImGui::End();   // Begin の戻り値が false（たたまれている）でも End は必ず呼ぶ
}

void ParticleEditorWindow::DrawWorkspace(const EditorContext& context) {
	// パーティクルのワークスペースでは、EditorManager のホストウィンドウの中に直接描く
	// （Begin("パーティクルエディタ") とは別の場所なので、ドッキングの配置とぶつからない）
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
	ImGui::BeginChild("##particleWorkspace", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding);
	ImGui::PopStyleVar();
	DrawContents(context);
	ImGui::EndChild();
}

void ParticleEditorWindow::DrawContents(const EditorContext& context) {
	// System は SceneContext の renderer から取る
	GpuParticleSystem* system = nullptr;
	if (context.sceneContext && context.sceneContext->renderer) {
		system = context.sceneContext->renderer->GetParticleSystem();
	}

	if (!system) {
		ImGui::TextDisabled("パーティクルシステムがありません");
	}
	else {
		// 左：一覧（200px は最初の幅。右端をドラッグして変えられ、幅は imgui.ini に保存される）
		ImGui::BeginChild("##list", ImVec2(200.0f, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
		DrawEffectList(*system);
		ImGui::EndChild();

		ImGui::SameLine();

		ParticleEffectData* data = system->FindEffect(selectedName_);
		if (!data) {
			ImGui::BeginChild("##empty");
			ImGui::TextDisabled("左の一覧からエフェクトを選んでください");
			ImGui::EndChild();
		}
		else {
			// 残りの幅を「プレビュー列」と「パラメータ列」に分ける
			// ・パラメータ列だけを子ウィンドウでスクロールさせるので、プレビューは常に見えたまま
			// ・プレビュー列も右端をドラッグして幅を変えられる（380px は最初の幅）

			// 中：プレビュー（ここはスクロールさせない）
			ImGui::BeginChild("##preview", ImVec2(380.0f, 0.0f),
							  ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX,
							  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			DrawPreview(*system, context.sceneContext->renderer);
			ImGui::EndChild();

			ImGui::SameLine();

			// 右：パラメータ（幅 0 = 残り全部なので、左2つの幅に合わせて自動で伸び縮みする）
			ImGui::BeginChild("##params", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
			if (DrawParameters(*system, *data)) {
				system->ApplyEffect(selectedName_);   // 変わったフレームだけ反映
				unsavedNames_.insert(selectedName_);
			}
			ImGui::EndChild();
		}
	}
}

CameraData ParticleEditorWindow::MakePreviewCamera() const {
	Vector3 target = previewPosition_;
	Vector3 eye = {
		target.x + orbitDistance_ * std::cos(orbitPitch_) * std::sin(orbitYaw_),
		target.y + orbitDistance_ * std::sin(orbitPitch_),
		target.z - orbitDistance_ * std::cos(orbitPitch_) * std::cos(orbitYaw_),
	};

	CameraData camera{};
	camera.view  = Math::MakeLookAtMatrix(eye, target, { 0.0f, 1.0f, 0.0f });
	camera.world = Math::Inverse(camera.view);   // ビルボードは world の1行目（右）・2行目（上）を使う
	camera.proj  = Math::MakePerspectiveFOVMatrix(0.8f, 1.0f, 0.1f, 1000.0f);   // 正方形なのでアスペクト比 1
	camera.vp    = Math::Multiply(camera.view, camera.proj);
	return camera;
	return CameraData();
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

void ParticleEditorWindow::RequestPreviewGrid() const {
	LineRenderer* line = LineRenderer::GetInstance();
	const Vector3& c = previewPosition_;
	float half = gridSize_ * 0.5f;

	// 地面のグリッド（プレビュー位置の高さ）
	line->DrawPreviewGridXZ(c, gridSize_, gridStep_, { 0.5f, 0.5f, 0.55f, 0.35f });

	// 中心の軸だけ色を付けて向きを分かりやすくする（X = 赤、Z = 青）
	line->DrawPreviewLine({ c.x - half, c.y, c.z }, { c.x + half, c.y, c.z }, { 0.9f, 0.3f, 0.3f, 0.8f });
	line->DrawPreviewLine({ c.x, c.y, c.z - half }, { c.x, c.y, c.z + half }, { 0.3f, 0.5f, 0.9f, 0.8f });
}

void ParticleEditorWindow::DrawPreview(GpuParticleSystem& system, MyEngine::Rendering::Renderer* renderer) {
	// プレビューを頼む（描かれるのはこの後の RenderScene）
	renderer->RequestParticlePreview(selectedName_, MakePreviewCamera());

	// グリッドの線もこのフレームのうちに要求しておく（Renderer がプレビューの粒より先に描く）
	if (showGrid_) {
		RequestPreviewGrid();
	}

	// 幅に合わせた正方形（大きすぎないように 360px まで）
	float size = std::min(ImGui::GetContentRegionAvail().x, float(MyEngine::Rendering::Renderer::kParticlePreviewSize));
	ImVec2 pos = ImGui::GetCursorScreenPos();

	// 画像のアルファが 1 未満のところ（消えかけの粒）でウィンドウの背景が透けないよう、同じ色を先に塗る
	ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size, pos.y + size), IM_COL32(20, 20, 26, 255));
	D3D12_GPU_DESCRIPTOR_HANDLE handle = renderer->GetParticlePreviewSrvHandle();
	ImGui::Image((ImTextureID)handle.ptr, ImVec2(size, size));

	// 画像と同じ場所に透明なボタンを重ねて、マウス操作を受け取る
	// ・Image は ID を持たない（操作を受け取れない）ので、ID を持つ InvisibleButton を使う
	// ・ボタンを押している間は「アクティブ」になるので、画像の外までドラッグしても回し続けられる
	ImGui::SetCursorScreenPos(pos);
	ImGui::InvisibleButton("##previewView", ImVec2(size, size));

	// ホイールをこのボタンの持ち物にする。これで親の子ウィンドウ（パラメータ欄）がスクロールしなくなる
	ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);

	ImGuiIO& io = ImGui::GetIO();
	if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
		orbitYaw_   -= io.MouseDelta.x * 0.01f;   // 右にドラッグすると右に回り込む向き
		orbitPitch_ += io.MouseDelta.y * 0.01f;
		orbitPitch_ = std::clamp(orbitPitch_, -1.5f, 1.5f);
	}
	if (ImGui::IsItemHovered()) {
		orbitDistance_ = std::clamp(orbitDistance_ - io.MouseWheel * 0.5f, 1.0f, 100.0f);
	}

	ImGui::Text("編集中: %s", selectedName_.c_str());

	// バースト再生と、出し続けるプレビューを出す位置
	ImGui::DragFloat3("プレビュー位置", &previewPosition_.x, 0.05f);

	// グリッドの表示切り替え（開いたときだけ大きさ・間隔を変えられる）
	ImGui::Checkbox("グリッド表示", &showGrid_);
	if (showGrid_) {
		ImGui::SameLine();
		ImGui::SetNextItemWidth(80.0f);
		ImGui::DragFloat("大きさ##grid", &gridSize_, 0.1f, 1.0f, 100.0f);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(80.0f);
		ImGui::DragFloat("間隔##grid", &gridStep_, 0.05f, 0.1f, 10.0f);
	}

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
