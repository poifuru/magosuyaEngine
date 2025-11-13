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
}

void PlayScene::Update () {
	
}

void PlayScene::Draw () {
	
}