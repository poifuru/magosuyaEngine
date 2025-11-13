#pragma once
#include "Scene.h"
#include <memory>
#include "object/2d/Sprite.h"
#include "object/3d/Model.h"

class GameoverScene : public Scene {
public:		//メンバ関数
	GameoverScene (MagosuyaEngine* magosuya);
	~GameoverScene () override;

	void Initialize () override;
	void Update () override;
	void Draw () override;

private:	//メンバ変数
	
};