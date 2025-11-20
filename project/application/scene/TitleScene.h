#pragma once
#include "Scene.h"
#include <memory>
#include "object/2d/Sprite.h"
#include "object/3d/Model.h"

class TitleScene : public Scene {
public:		//メンバ関数
	TitleScene (MagosuyaEngine* magosuya);
	~TitleScene () override;

	void Initialize () override;
	void Update () override;
	void Draw () override;

private:	//メンバ変数
	std::unique_ptr<CameraData> camera_ = nullptr;
	std::unique_ptr<Model> model_ = nullptr;
};
