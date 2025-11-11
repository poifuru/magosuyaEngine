#pragma once
#include "Scene.h"
#include <memory>
#include "../../magosuya/object/2d/Sprite.h"

class MagosuyaEngine;

class ClearScene : public Scene {
public:		//メンバ関数
	ClearScene (MagosuyaEngine* magosuya);
	~ClearScene () override;

	void Initialize () override;
	void Update () override;
	void Draw () override;

private:	//メンバ変数
	std::unique_ptr<Sprite> clear_ = nullptr;

	//ポインタ貸し出し
	MagosuyaEngine* magosuya_ = nullptr;
};