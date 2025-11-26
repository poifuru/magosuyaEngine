#pragma once
#include "Scene.h"
#include <memory>
#include "Sprite.h"
#include "Model.h"

class PlayScene : public Scene {
public:		//メンバ関数
	PlayScene (MagosuyaEngine* magosuya);
	~PlayScene () override;

	void Initialize () override;
	void Update () override;
	void Draw () override;

private:	//メンバ変数

};
