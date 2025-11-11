#include "PlayScene.h"
#include "../../general/Math.h"
#include "../../magosuya/MagosuyaEngine.h"

PlayScene::PlayScene (MagosuyaEngine* magosuya) {
	magosuya_ = magosuya;
	cameraData_ = new CameraData;
	player_ = std::make_unique<Player> (magosuya, cameraData_);
	enemy_ = std::make_unique<Enemy> (magosuya, cameraData_);
	ground_ = std::make_unique<Sprite> (magosuya);
	info_ = std::make_unique<Sprite> (magosuya);
	magosuya_->LoadTexture ("Resources/Textures/ground.png", "ground");
	magosuya_->LoadTexture ("Resources/Textures/Info.png", "info");
}

PlayScene::~PlayScene () {
	delete cameraData_;
	magosuya_->UnloadTexture ("Resources/Textures/ground.png");
}

void PlayScene::Initialize () {
	nowScene_ = SceneLabel::Play;
	isFinish_ = false;

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
	info_->SetID ("info");
	info_->Initialize ({ 0.0f, 0.0f, 0.0f });
	info_->SetTexture (magosuya_->GetTextureHandle ("info"));
}

void PlayScene::Update () {
	cameraData_->World = MakeAffineMatrix (cameraData_->transform.scale, cameraData_->transform.rotate, cameraData_->transform.translate);
	cameraData_->View = Inverse (cameraData_->World);
	player_->Update ();
	enemy_->Update ();
	ground_->Update ();
	info_->Update ();
	Collision ();

	if (player_->GetHp() == 0) {
		nextScene_ = SceneLabel::Gameover;
		isFinish_ = true;
	}
	if (enemy_->GetHp () == 0) {
		nextScene_ = SceneLabel::Clear;
		isFinish_ = true;
	}
}

void PlayScene::Draw () {
	player_->Draw ();
	enemy_->Draw ();
	ground_->Draw ();
	info_->Draw ();
}

void PlayScene::Collision () {
	//プレイヤーと敵の当たり判定
	if (IsCollision (player_->GetAABBModel (), enemy_->GetAABBModel ())) {
		player_->IsHit ();
	}

	//武器と敵のあたり判定
	if (IsCollision (player_->GetAABBWeapon_ (), enemy_->GetAABBModel ()) && player_->GetIsAttack()) {
		if (!player_->GetAttackIsHit ()) {
			enemy_->IsHit (true);
			player_->SetAttackIsHit (true);
		}
	}
}