#include "PCH.h"
#include "BaseScene.h"
#include "EditorManager.h"

void BaseScene::Update(CameraData* cameraData) {
	// エディタの再生ステートを判定
	if (EditorManager::GetInstance()->IsPlaying()) {
		// 再生中：ゲーム固有の更新を動かす
		UpdateGame(cameraData);
	}
	else {
		// 停止中・一時停止中：エディタ時の更新（行列計算やカメラ）だけ動かす
		UpdateEdit(cameraData);
	}
}

void BaseScene::UpdateEdit(CameraData* cameraData) {
}
