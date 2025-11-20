#include "Boss.h"
#include "MathFunction.h"
#include <imgui.h>

Boss::Boss(MagosuyaEngine* magosuya) {
	magosuya_ = magosuya;
	model_ = std::make_unique<Model>(magosuya);
	magosuya_->LoadModelData("Resources/teapot", "teapot");
}

Boss::~Boss() {
	
}

void Boss::Initialize() {
	model_->SetModelData("teapot");
	model_->SetTexture("teapot");
	model_->Initialize();

	centerStomp_ = std::make_unique<CenterStomp>(magosuya_, this);
	centerStomp_->Initialize();

	transform_ = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
	speed_ = { 0.1f,0.1f,0.1f };
}

void Boss::Update(Matrix4x4* m) {
	// 行動の更新
	UpdateMove();

	if (magosuya_->GetRawInput()->Trigger('1')) {
		centerStomp_->StartAttack();
	}

	model_->Update(m);
	centerStomp_->Update(m);

	model_->SetTransform(transform_);
}

void Boss::Draw() {
	model_->Draw();
	centerStomp_->Draw();
}

void Boss::ImGuiControl() {
#ifdef _DEBUG
	ImGui::Begin("Boss");
	ImGui::DragFloat3("Scale", &transform_.scale.x, 0.1f);
	ImGui::DragFloat3("Rotate", &transform_.rotate.x, 0.1f);
	ImGui::DragFloat3("Translate", &transform_.translate.x, 0.1f);
	ImGui::End();

	centerStomp_->ImGuiControl();
#endif
}

void Boss::UpdateMove() {
	if (centerStomp_->IsAttacking()) {
		return;
	}
	transform_.translate.x += static_cast<float>(rand() % 3 - 1) * 0.1f;
	transform_.translate.z += static_cast<float>(rand() % 3 - 1) * 0.1f;
}