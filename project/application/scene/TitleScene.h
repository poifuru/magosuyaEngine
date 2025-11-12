
#pragma once
#include <memory>
#include "Scene.h"
#include "../../magosuya/object/2d/Sprite.h"

class MagosuyaEngine;

class TitleScene : public Scene {
public:		//メンバ関数
	TitleScene (MagosuyaEngine* magosuya);
	~TitleScene () override;

	void Initialize () override;
	void Update () override;
	void Draw () override;

private:	//メンバ変数
	std::unique_ptr<Sprite> title_ = nullptr;

	//ポインタ貸し出し
	MagosuyaEngine* magosuya_ = nullptr;
};
