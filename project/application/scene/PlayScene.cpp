#include "PlayScene.h"
#include "../../general/Math.h"
#include "../../magosuya/MagosuyaEngine.h"

PlayScene::PlayScene (MagosuyaEngine* magosuya) {
	magosuya_ = magosuya;
	cameraData_ = new CameraData;
	player_ = std::make_unique<Player> (magosuya, cameraData_);
	enemy_ = std::make_unique<Enemy> (magosuya, cameraData_);
	ground_ = std::make_unique<Sprite> (magosuya);
	magosuya_->LoadTexture ("Resources/Textures/ground.png", "ground");
}

PlayScene::~PlayScene () {
	delete cameraData_;
	magosuya_->UnloadTexture ("Resources/Textures/ground.png");
}

void PlayScene::Initialize () {
	nowScene_ = SceneLabel::Play;

	//カメラ作成
	cameraData_->transform = { {1.0f, 1.0f, 1.0f}, {}, {0.0f, 0.0f, -50.0f} };
	cameraData_->World = MakeIdentity4x4 ();
	cameraData_->View = MakeIdentity4x4 ();
	cameraData_->Proj = MakePerspectiveFOVMatrix (0.45f, float (magosuya_->GetDxCommon ()->GetWinAPI ()->kClientWidth) / float (magosuya_->GetDxCommon ()->GetWinAPI ()->kClientHeight), 0.1f, 100.0f);
	player_->Initialize ();
	enemy_->Initialize ();

	ground_->SetID ("ground");
	ground_->Initialize ({ 0.0f, 420.0f, 0.0f });
	ground_->SetTexture (magosuya_->GetTextureHandle ("ground"));
}

void PlayScene::Update () {
	cameraData_->World = MakeAffineMatrix (cameraData_->transform.scale, cameraData_->transform.rotate, cameraData_->transform.translate);
	cameraData_->View = Inverse (cameraData_->World);
	player_->Update ();
	enemy_->Update ();
	ground_->Update ();

	if (player_->GetHp() == 0) {
		nextScene_ = SceneLabel::Gameover;
		isFinish_ = true;
	}
}

void PlayScene::Draw () {
	player_->Draw ();
	enemy_->Draw ();
	ground_->Draw ();
}