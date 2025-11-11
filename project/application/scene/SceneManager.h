#pragma once
#include <memory>
#include <string>
#include "Scene.h"
#include "TitleScene.h"
#include "PlayScene.h"
#include "ClearScene.h"

class MagosuyaEngine;

// ゲーム全体の状態を定義
enum class GameState {
	Running, // 通常実行中
	Paused   // ポーズ中
};

class SceneManager {
public:		//メンバ関数
	SceneManager (MagosuyaEngine* magosuya);
	~SceneManager ();

	void Initialize (SceneLabel scene);
	void Update ();
	void Draw ();

private:	//メンバ変数
	//それぞれのシーンのポインタ
	std::unique_ptr<TitleScene> titleScene_;
	std::unique_ptr<PlayScene> playScene_;
	std::unique_ptr<ClearScene> clearScene_;

	//現在のシーン
	SceneLabel scene_;
	//処理を共通化するためのポインタ
	Scene* currentScene_;

	//ポインタ貸し出し
	MagosuyaEngine* magosuya_ = nullptr;
};

