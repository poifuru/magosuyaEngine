#include "GameoverScene.h"
#include "MagosuyaEngine.h"

GameoverScene::GameoverScene (MagosuyaEngine* magosuya) {
	magosuya_ = magosuya;
}

GameoverScene::~GameoverScene () {
	
}

void GameoverScene::Initialize () {
	nowScene_ = SceneLabel::Gameover;
	isFinish_ = false;
}

void GameoverScene::Update () {
	
}

void GameoverScene::Draw () {
	
}
