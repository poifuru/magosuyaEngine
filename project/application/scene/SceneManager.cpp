#include "SceneManager.h"

SceneManager::SceneManager (MagosuyaEngine* magosuya) {
	magosuya_ = magosuya;
	scene_ = SceneLabel::Title;
	titleScene_ = std::make_unique<TitleScene> (magosuya);
	playScene_ = std::make_unique<PlayScene> (magosuya);
	clearScene_ = std::make_unique<ClearScene> (magosuya);
	gameoverScene_ = std::make_unique<GameoverScene> (magosuya);
	currentScene_ = titleScene_.get ();
}

SceneManager::~SceneManager () {

}

void SceneManager::Initialize (SceneLabel scene) {
	//引数で初期化のシーンを選択
	switch (scene) {
	case SceneLabel::Title:
		currentScene_ = titleScene_.get ();
		break;

	case SceneLabel::Play:
		currentScene_ = playScene_.get ();
		break;

	case SceneLabel::Clear:
		currentScene_ = clearScene_.get ();
		break;

	case SceneLabel::Gameover:
		currentScene_ = gameoverScene_.get ();
		break;
	}
	currentScene_->Initialize ();
}

void SceneManager::Update () {
	currentScene_->Update ();

	if (currentScene_->GetIsFinish ()) {
		switch (currentScene_->GetNextScene ()) {
		case SceneLabel::Title:
			currentScene_ = titleScene_.get ();
			break;

		case SceneLabel::Play:
			currentScene_ = playScene_.get ();
			break;

		case SceneLabel::Clear:
			currentScene_ = clearScene_.get ();
			break;

		case SceneLabel::Gameover:
			currentScene_ = gameoverScene_.get ();
			break;
		}
		currentScene_->Initialize ();
	}
}

void SceneManager::Draw () {
	currentScene_->Draw ();
}