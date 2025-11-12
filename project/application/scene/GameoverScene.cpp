
#include "GameoverScene.h"
#include "MagosuyaEngine.h"
#include "utility/input/InputManager.h"

GameoverScene::GameoverScene (MagosuyaEngine* magosuya) {
	magosuya_ = magosuya;
	gameover_ = std::make_unique<Sprite> (magosuya);
}

GameoverScene::~GameoverScene () {
	//画像のアンロード
	magosuya_->UnloadTexture ("Resources/Textures/gameover.png");
}

void GameoverScene::Initialize () {
	nowScene_ = SceneLabel::Gameover;
	isFinish_ = false;
	//画像のロード
	magosuya_->LoadTexture ("Resources/Textures/gameover.png", "gameover");
	gameover_->SetID ("gameover");
	gameover_->Initialize ({ 0.0f, 0.0f, 0.0f });
	gameover_->SetTexture (magosuya_->GetTextureHandle ("gameover"));
}

void GameoverScene::Update () {
	if (g_inputManager->GetRawInput ()->Trigger (VK_SPACE)) {
		nextScene_ = SceneLabel::Title;
		isFinish_ = true;
	}

	gameover_->Update ();
}

void GameoverScene::Draw () {
	gameover_->Draw ();
}
