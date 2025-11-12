
#pragma once
#include "Scene.h"
#include "../player/Player.h"
#include "../enemy/Enemy.h"
#include "../../magosuya/object/2d/Sprite.h"

class MagosuyaEngine;

class PlayScene : public Scene {
public:		//メンバ関数
	PlayScene (MagosuyaEngine* magosuya);
	~PlayScene () override;

	void Initialize () override;
	void Update () override;
	void Draw () override;

private:
	void Collision ();

public:		//アクセッサ
	CameraData* GetCamera () { return cameraData_; }

private:	//メンバ変数
	//カメラ
	CameraData* cameraData_ = nullptr;

	//プレイヤー
	std::unique_ptr<Player> player_ = nullptr;
	std::unique_ptr<Enemy> enemy_ = nullptr;

	//地面
	std::unique_ptr<Sprite> ground_ = nullptr;
	std::unique_ptr<Sprite> info_ = nullptr;
	//ポインタ貸し出し
	MagosuyaEngine* magosuya_ = nullptr;
};
