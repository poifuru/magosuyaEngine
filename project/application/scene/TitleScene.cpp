
#include "TitleScene.h"
#include <cassert>
#include <imgui.h>
#include "MagosuyaEngine.h"
#include "utility/input/InputManager.h"

TitleScene::TitleScene (MagosuyaEngine* magosuya) {
	magosuya_ = magosuya;
	title_ = std::make_unique<Sprite> (magosuya_);
}

TitleScene::~TitleScene () {
	//画像のアンロード
	magosuya_->UnloadTexture ("Resources/Textures/Title.png");
}

void TitleScene::Initialize () {
	//画像のロード
	magosuya_->LoadTexture ("Resources/Textures/Title.png", "title");

	nowScene_ = SceneLabel::Title;
	isFinish_ = false;
	title_->SetID ("title");
	title_->Initialize ({ 0.0f, 0.0f, 0.0f });
	title_->SetTexture (magosuya_->GetTextureHandle ("title"));
}

void TitleScene::Update () {
	if (g_inputManager->GetRawInput ()->Trigger (VK_SPACE)) {
		nextScene_ = SceneLabel::Play;
		isFinish_ = true;
	}

	title_->Update ();
}

void TitleScene::Draw () {
	title_->Draw ();
}
