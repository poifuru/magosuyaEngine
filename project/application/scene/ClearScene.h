#pragma once
#include "Scene.h"

class MagosuyaEngine;

class ClearScene : public Scene {
public:		//メンバ関数
	ClearScene (MagosuyaEngine* magosuya);
	~ClearScene () override;

	void Initialize () override;
	void Update () override;
	void Draw () override;

private:	//メンバ変数

	//ポインタ貸し出し
	MagosuyaEngine* magosuya_ = nullptr;
};