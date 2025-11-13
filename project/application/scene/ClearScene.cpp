#include "ClearScene.h"
#include "MagosuyaEngine.h"

ClearScene::ClearScene (MagosuyaEngine* magosuya) {
	magosuya_ = magosuya;
}

ClearScene::~ClearScene () {
	
}

void ClearScene::Initialize () {
	nowScene_ = SceneLabel::Clear;
	isFinish_ = false;
}

void ClearScene::Update () {

}

void ClearScene::Draw () {
	
}