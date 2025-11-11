#include "ClearScene.h"
#include "../../magosuya/MagosuyaEngine.h"
#include "../../magosuya/utility/input/InputManager.h"

ClearScene::ClearScene (MagosuyaEngine* magosuya) {
	magosuya_ = magosuya;
	clear_ = std::make_unique<Sprite> (magosuya);
}

ClearScene::~ClearScene () {
	//画像のアンロード
	magosuya_->UnloadTexture ("Resources/Textures/clear.png");
}

void ClearScene::Initialize () {
	nowScene_ = SceneLabel::Clear;
	isFinish_ = false;
	//画像のロード
	magosuya_->LoadTexture ("Resources/Textures/clear.png", "clear");
	clear_->SetID ("clear");
	clear_->Initialize ({ 0.0f, 0.0f, 0.0f });
	clear_->SetTexture (magosuya_->GetTextureHandle ("clear"));
}

void ClearScene::Update () {
	if (g_inputManager->GetRawInput ()->Trigger (VK_SPACE)) {
		nextScene_ = SceneLabel::Title;
		isFinish_ = true;
	}

	clear_->Update ();
}

void ClearScene::Draw () {
	clear_->Draw ();
}
