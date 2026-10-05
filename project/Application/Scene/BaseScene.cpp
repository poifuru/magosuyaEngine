#include "PCH.h"
#include "BaseScene.h"
#include "EditorManager.h"

void BaseScene::Update(CameraData* cameraData) {
#ifdef USEIMGUI
	// エディタがある開発ビルド：Play中ならゲーム、停止中ならエディタ更新
	if (EditorManager::GetInstance()->IsPlaying()) {
		UpdateGame(cameraData);
	}
	else {
		UpdateEdit(cameraData);
	}
#else
	// リリースビルド：エディタはないので、最初から常にゲーム本編を実行！
	UpdateGame(cameraData);
#endif
}

void BaseScene::UpdateEdit(CameraData* cameraData) {
}
