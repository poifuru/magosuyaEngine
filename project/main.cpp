#include <Windows.h>
#include "magosuya/MagosuyaEngine.h"
#include "general/function.h"
#include "general/Math.h"
#include "general/struct.h"
#include <format>// C++20のformat() 文字列整形
#include <chrono>	//時間を扱うライブラリ
#include <sstream>// stringstream
#include <memory>
#include <xaudio2.h>
#pragma comment(lib,"xaudio2.lib")
#include <Xinput.h>
#pragma comment(lib, "xinput.lib")
#include "magosuya/object/3d/Model.h"
#include "magosuya/object/3d/SphereModel.h"
#include "magosuya/object/2d/Sprite.h"
#include "magosuya/utility/camera/DebugCamera.h"
#include "magosuya/utility/Input/InputManager.h"

//サウンドデータの読み込み関数
SoundData SoundLoadWave (const char* filename) {
	/*1,ファイルを開く*/
	//ファイルストリームのインスタンス
	std::ifstream file;
	//wavファイルをバイナリーモードで開く
	file.open (filename, std::ios_base::binary);
	//ファイルが開けなければassert
	assert (file.is_open ());

	/*2,wavデータ読み込み*/
	//RIFFヘッダーの読み込み
	RiffHeader riff;
	file.read ((char*)&riff, sizeof (riff));
	//ファイルがRIFFかチェック
	if (strncmp (riff.chunk.id, "RIFF", 4) != 0) {
		assert (0);
	}
	//タイプがWAVEかチェック
	if (strncmp (riff.type, "WAVE", 4) != 0) {
		assert (0);
	}

	//Formatチャンクの読み込み
	FormatChunk format = {};
	//チャンクヘッダーの確認
	file.read ((char*)&format, sizeof (ChunkHeader));
	if (strncmp (format.chunk.id, "fmt ", 4) != 0) {
		assert (0);
	}

	//チャンク本体の読み込み
	assert (format.chunk.size <= sizeof (format.fmt));
	file.read ((char*)&format.fmt, format.chunk.size);

	//Dataチャンクの読み込み
	ChunkHeader data;
	file.read ((char*)&data, sizeof (data));
	//JUNKチャンクを検出した場合
	if (strncmp (data.id, "JUNK", 4) != 0) {
		//読み取り位置をJUNKチャンクの終わりまで進める
		file.seekg (data.size, std::ios_base::cur);
		//再読み込み
		file.read ((char*)&data, sizeof (data));
	}

	if (strncmp (data.id, "data", 4) != 0) {
		assert (0);
	}

	//Dataチャンクのデータ部(波形データ)の読み込み
	char* pBuffer = new char[data.size];
	file.read (pBuffer, data.size);

	//waveファイルを閉じる
	file.close ();

	//returnするための音声データ
	SoundData soundData = {};

	soundData.wfex = format.fmt;
	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
	soundData.bufferSize = data.size;

	return soundData;
}

//音声データを解放する関数
void SoundUnload (SoundData* soundData) {
	//バッファのメモリを解放
	delete[] soundData->pBuffer;

	soundData->pBuffer = 0;
	soundData->bufferSize = 0;
	soundData->wfex = {};
}

//音声再生の関数
void SoundPlayWave (IXAudio2* xAudio2, const SoundData& soundData) {
	HRESULT result;

	//波形フォーマットを元にSourceVoiceの生成
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	result = xAudio2->CreateSourceVoice (&pSourceVoice, &soundData.wfex);
	assert (SUCCEEDED (result));

	//再生する波形データの設定
	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.pBuffer;
	buf.AudioBytes = soundData.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;

	//波形データの再生
	result = pSourceVoice->SubmitSourceBuffer (&buf);
	result = pSourceVoice->Start ();
}

std::unique_ptr<InputManager> g_inputManager = nullptr;

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain (HINSTANCE, HINSTANCE, LPSTR, int) {
	std::unique_ptr<MagosuyaEngine> magosuya = std::make_unique<MagosuyaEngine> ();

	HRESULT hr;

	//サウンドの導入
	ComPtr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice* masterVoice;

	hr = XAudio2Create (&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert (SUCCEEDED (hr));

	//マスターボイスを生成
	hr = xAudio2->CreateMasteringVoice (&masterVoice);

	//音声の読み込み
	SoundData soundData1 = SoundLoadWave ("Resources/Sounds/Alarm01.wav");

#pragma region Plane
	std::unique_ptr<Model> plane = std::make_unique<Model> (magosuya->GetDxCommon(), "Resources/plane", "plane", true);
#pragma endregion

#pragma region bunny
	std::unique_ptr<Model> bunny = std::make_unique<Model> (magosuya->GetDxCommon (), "Resources/bunny", "bunny", false);
#pragma endregion

#pragma region Teapot
	std::unique_ptr<Model> teapot = std::make_unique<Model> (magosuya->GetDxCommon (), "Resources/teapot", "teapot", false);
#pragma endregion

#pragma region Fence
	std::unique_ptr<Model> Fence = std::make_unique<Model> (magosuya->GetDxCommon (), "Resources/fence", "fence", false);
#pragma endregion

	//平行光源のResourceを作成してデフォルト値を書き込む
	ComPtr<ID3D12Resource> dierctionalLightResource = magosuya->GetDxCommon()->CreateBufferResource (sizeof (DirectionalLight));
	DirectionalLight* directionalLightData = nullptr;
	//書き込むためのアドレス取得
	dierctionalLightResource->Map (0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	//実際に書き込み
	directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionalLightData->direction = { 0.0f, -1.0f, 0.0f };
	directionalLightData->intensity = 1.0f;
	directionalLightData->mode = Light::halfLambert;

	//Transform
	Transform transform{ {1.0f, 1.0f, 1.0f}, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
	Transform transformModel{ {1.0f, 1.0f, 1.0f}, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
	Transform transformTeapot{ {1.0f, 1.0f, 1.0f}, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
	Transform cameraTransform{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -10.0f} };
	Transform transformSprite{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

	//UVtransform用の変数
	Transform uvTransformSprite{
		{1.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f},
	};

	//DescriptorSizeを取得しておく
	const uint32_t descriptorSizeSRV = magosuya->GetDxCommon ()->GetDevice ()->GetDescriptorHandleIncrementSize (D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t descriptorSizeRTV = magosuya->GetDxCommon ()->GetDevice ()->GetDescriptorHandleIncrementSize (D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t descriptorSizeDSV = magosuya->GetDxCommon ()->GetDevice ()->GetDescriptorHandleIncrementSize (D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	//テクスチャの読み込み
	magosuya->LoadTexture ("Resources/uvChecker.png", "uvChecker");
	magosuya->LoadTexture ("Resources/monsterBall.png", "monsterBall");

	//BGM再生
	SoundPlayWave (xAudio2.Get (), soundData1);

	//スプライト
	std::unique_ptr<Sprite> sprite = std::make_unique<Sprite> (magosuya->GetDxCommon());
	sprite->Initialize ({ 0.0f, 0.0f, 0.0f }, { 640.0f, 360.0f });
	sprite->SetTexture (magosuya->GetTextureHandle ("uvChecker"));

	std::unique_ptr<SphereModel> sphere = std::make_unique<SphereModel> (magosuya->GetDxCommon (), 16);
	sphere->Initialize ({ 0.0f, 0.0f, 0.0f }, 1.0f);

	plane->Initialize ();
	bunny->Initialize ();
	teapot->Initialize ();
	Fence->Initialize ();

	//カメラ用
	Matrix4x4 cameraMatrix = {};
	Matrix4x4 viewMatrix = {};
	Matrix4x4 projectionMatrix = {};

	//デバッグカメラ
	std::unique_ptr<DebugCamera> debugCamera = std::make_unique<DebugCamera> ();
	debugCamera->Initialize ();
	bool debugMode = false;

	//テクスチャ切り替え用の変数
	bool useMonsterBall = true;
	//スプライト切り替え
	bool useSprite = false;
	//球の切り替え
	bool useSphere = false;
	//ぷれーん
	bool usePlane = true;
	//うさぎ
	bool useModel = false;
	//てぃーぽっと
	bool useTeapot = false;

	//ライティング用の変数
	float colorLight[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	//お試し
	Vector3 pos{};

	/*********************************/

	/*メインループ！！！！！！！！！*/
	//ウィンドウの×ボタンが押されるまでループ
	while (true) {

		if (magosuya->GetDxCommon ()->GetWinAPI ()->ProcessMessage ()) {
			break;
		}

		//フレーム開始
		magosuya->BeginFrame ();
		//FPS表示
		ImGui::Begin ("Debug Window");
		ImGui::Text ("FPS: %.1f", ImGui::GetIO ().Framerate);
		ImGui::End ();

		//実際のキー入力処理はここ！
		// 押した瞬間だけトグル
		if (g_inputManager->GetRawInput ()->Trigger (VK_TAB)) {
			if (!debugMode) {
				debugMode = true;
			}
			else {
				debugMode = false;
			}
		}

		if (g_inputManager->GetRawInput ()->Push ('D')) {
			pos.x += 0.01f;
		}
		ImGui::Text ("pos.x:%f", pos.x);

		//ゲームの処理//
		//=======オブジェクトの更新処理=======//
		//カメラ
		cameraMatrix = MakeAffineMatrix (cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
		viewMatrix = Inverse (cameraMatrix);
		projectionMatrix = MakePerspectiveFOVMatrix (0.45f, float (magosuya->GetDxCommon ()->GetWinAPI ()->kClientWidth) / float (magosuya->GetDxCommon ()->GetWinAPI ()->kClientHeight), 0.1f, 100.0f);
		if (!debugMode) {
			//worldViewProjectionMatrix = Multiply (cameraMatrix, Multiply (viewMatrix, projectionMatrix));
		}
		if (!ImGui::GetIO ().WantCaptureMouse) {
			if (debugMode) {
				debugCamera->Updata (magosuya->GetDxCommon ()->GetWinAPI ()->GetHwnd (), hr, g_inputManager.get ());
				viewMatrix = debugCamera->GetViewMatrix ();
				projectionMatrix = debugCamera->GetProjectionMatrix ();
			}
		}

		//オブジェクト
		plane->Update (&viewMatrix, &projectionMatrix);
		plane->SetPositon (pos);

		bunny->Update (&viewMatrix, &projectionMatrix);

		teapot->Update (&viewMatrix, &projectionMatrix);

		Fence->Update (&viewMatrix, &projectionMatrix);

		sprite->Update ();

		sphere->Update (&viewMatrix, &projectionMatrix);

		//光源のdirectionの正規化
		directionalLightData->direction = Normalize (directionalLightData->direction);

		ImGui::Begin ("カメラモード:TAB");
		if (debugMode) {
			ImGui::TextColored (ImVec4 (1, 1, 0, 1), "Current Camera: Debug");
		}
		else if (!debugMode) {
			ImGui::TextColored (ImVec4 (0, 1, 0, 1), "Current Camera: Scene");
		}
		ImGui::End ();

		//ImGuiと変数を結び付ける
		// 色変更用のUI
		static float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };  // 初期値：白

		ImGui::Begin ("setting");
		if (ImGui::CollapsingHeader ("SceneCamera")) {
			if (ImGui::Button ("Reset")) {
				cameraTransform = {
					{1.0f, 1.0f, 1.0f},
					{0.0f, 0.0f, 0.0f},
					{0.0f, 0.0f, -10.0f},
				};
			}
			ImGui::DragFloat3 ("cameraScale", &cameraTransform.scale.x, 0.01f);
			ImGui::DragFloat3 ("cameraRotate", &cameraTransform.rotate.x, 0.01f);
			ImGui::DragFloat3 ("cameraTranslate", &cameraTransform.translate.x, 0.01f);
		}
		if (ImGui::CollapsingHeader ("sphere")) {
			ImGui::Checkbox ("speher##useSphere", &useSphere);
			sphere->ShowImGuiEditor ();
		}
		if (ImGui::CollapsingHeader ("plane")) {
			ImGui::Checkbox ("Draw##plane", &usePlane);
			plane->ImGui ();
		}
		if (ImGui::CollapsingHeader ("Model")) {
			ImGui::Checkbox ("Draw##Model", &useModel);
			bunny->ImGui ();
		}
		if (ImGui::CollapsingHeader ("teapod")) {
			ImGui::Checkbox ("Draw##teapod", &useTeapot);
			teapot->ImGui ();
		}
		if (ImGui::CollapsingHeader ("Sprite")) {
			ImGui::Checkbox ("Draw##useSprite", &useSprite);
			sprite->ImGui ();
		}
		if (ImGui::CollapsingHeader ("light")) {
			if (ImGui::ColorEdit4 ("color", colorLight)) {
				// 色が変更されたらmaterialDataに反映
				directionalLightData->color.x = colorLight[0];
				directionalLightData->color.y = colorLight[1];
				directionalLightData->color.z = colorLight[2];
				directionalLightData->color.w = colorLight[3];
			}
			ImGui::DragFloat3 ("lightDirection", &directionalLightData->direction.x, 0.01f);
			ImGui::DragFloat ("intensity", &directionalLightData->intensity, 0.01f);
		}
		if (ImGui::CollapsingHeader ("fence")) {
			//Fence->ImGui ();
		}
		ImGui::End ();
		ImGui::DragFloat2 ("UVTranslate", &uvTransformSprite.translate.x, 0.01f, -10.0f, 10.0f);
		ImGui::DragFloat2 ("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
		ImGui::SliderAngle ("UVRotate", &uvTransformSprite.rotate.z);



		//=======コマンド君達=======//
		//ライティングの設定
		magosuya->GetDxCommon ()->GetCommandList ()->SetGraphicsRootConstantBufferView (3, dierctionalLightResource->GetGPUVirtualAddress ());
		//描画！(DrawCall/ドローコール)。3頂点で1つのインスタンス。インスタンスについては今後
		Fence->Draw (*magosuya->GetTextureManger()->GetTextureHandle("uvChecker"));
		if (useSphere) {
			sphere->Draw (*magosuya->GetTextureManger ()->GetTextureHandle ("monsterBall"));
		}
		if (usePlane) {
			plane->Draw (*magosuya->GetTextureManger ()->GetTextureHandle ("monsterBall"));
		}
		if (useModel) {
			bunny->Draw (*magosuya->GetTextureManger ()->GetTextureHandle ("monsterBall"));
		}
		if (useTeapot) {
			teapot->Draw (*magosuya->GetTextureManger ()->GetTextureHandle ("uvChecker"));
		}
		if (useSprite) {
			sprite->Draw ();
		}

		//フレーム終了
		g_inputManager->EndFrame ();
		magosuya->EndFrame ();
	}
	
	xAudio2.Reset ();
	SoundUnload (&soundData1);  // バッファ解放
	magosuya->Finalize ();
	return 0;
};