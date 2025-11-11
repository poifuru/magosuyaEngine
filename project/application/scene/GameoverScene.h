#pragma once
#include "Scene.h"
#include <memory>
#include "../../magosuya/object/2d/Sprite.h"

class MagosuyaEngine;
class InputManager;
extern std::unique_ptr<InputManager> g_inputManager;

class GameoverScene : public Scene {
public:		//メンバ関数
	GameoverScene (MagosuyaEngine* magosuya);
	~GameoverScene () override;

	void Initialize () override;
	void Update () override;
	void Draw () override;

private:	//メンバ変数
	//std::unique_ptr<Sprite> title_ = nullptr;

	//ポインタ貸し出し
	MagosuyaEngine* magosuya_ = nullptr;
};

