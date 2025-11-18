#include "PlayScene.h"
#include "MagosuyaEngine.h"

PlayScene::PlayScene (MagosuyaEngine* magosuya) {
	magosuya_ = magosuya;
}

PlayScene::~PlayScene () {
	
}

void PlayScene::Initialize () {
	nowScene_ = SceneLabel::Play;
	isFinish_ = false;
	camera_ = std::make_unique<CameraData>();
	boss_ = std::make_unique<Boss>(magosuya_, camera_.get());
	boss_->Initialize();

	camera_->transform = {
		{1.0f, 1.0f, 1.0f},
		{0.6f, 0.0f, 0.0f},
		{0.0f, 10.0f, -20.0f},
	};
}

void PlayScene::Update () {
	boss_->ImGuiControl();

	if (magosuya_->GetRawInput()->Trigger(VK_SPACE)) {
		nextScene_ = SceneLabel::Title;
		isFinish_ = true;
	}

	boss_->Update();
}

void PlayScene::Draw () {
	boss_->Draw();
}