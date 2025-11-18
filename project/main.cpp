#include <Windows.h>
#include "MagosuyaEngine.h"
#include "SceneManager.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain (_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	std::unique_ptr<MagosuyaEngine> magosuya = std::make_unique<MagosuyaEngine> ();
	magosuya->Initialize ();

	std::unique_ptr<SceneManager> sceneManager = std::make_unique<SceneManager> (magosuya.get ());
	sceneManager->Initialize (SceneLabel::Title);

	//===メインループ===//
	//ウィンドウの×ボタンが押されるまでループ
	while (true) {

		if (magosuya->GetDxCommon ()->GetWinAPI ()->ProcessMessage ()) {
			break;
		}

		//フレーム開始
		magosuya->BeginFrame ();

		//***更新処理***//
		sceneManager->Update ();
		//*************//



		//***描画処理***//
		sceneManager->Draw ();
		//*************//

		//フレーム終了
		magosuya->EndFrame ();
	}

	magosuya->Finalize ();
	return 0;
};