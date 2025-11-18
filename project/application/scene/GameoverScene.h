#pragma once
#include "Scene.h"
#include <memory>
#include "Sprite.h"
#include "Model.h"

class GameoverScene : public Scene {
public:		//メンバ関数
	GameoverScene (MagosuyaEngine* magosuya);
	~GameoverScene () override;

	void Initialize () override;
	void Update () override;
	void Draw () override;

private:	//メンバ変数
	
};