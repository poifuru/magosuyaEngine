#include "TitleScene.h"
#include "MagosuyaEngine.h"
#include "utility/input/InputManager.h"

TitleScene::TitleScene (MagosuyaEngine* magosuya) {
	magosuya_ = magosuya;
}

TitleScene::~TitleScene () {

}

void TitleScene::Initialize () {
	nowScene_ = SceneLabel::Title;
	isFinish_ = false;
}

void TitleScene::Update () {

}

void TitleScene::Draw () {
	
}