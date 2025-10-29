#pragma once
#include "engine/engineCore/DxCommon.h"
#include "engine/engineCore/winAPI/WindowsAPI.h"

class poiEngine {
public:		//メンバ関数
	void Initialize ();

	//アクセッサ
	DxCommon* GetDxCommon () { return dxCommon_.get (); }
	WindowsAPI* GetWinAPI () { return winApp_.get(); }

private:	//メンバ変数
	std::unique_ptr<DxCommon> dxCommon_;
	std::unique_ptr<WindowsAPI> winApp_;
};