#include <Windows.h>
#include <format>// C++20のformat() 文字列整形
#include <chrono>	//時間を扱うライブラリ
#include <sstream>// stringstream
#include <memory>
#include <vector>
#include <xaudio2.h>
#pragma comment(lib,"xaudio2.lib")
#include <Xinput.h>
#pragma comment(lib, "xinput.lib")
#include <imgui.h>
#include "MagosuyaEngine.h"
#include "Windows.h"
#include "DxCommon.h"
#include "TextureManager.h"
#include "ImGuiManager.h"
#include "ModelManager.h"
#include "PSOManager.h"
#include "function.h"
#include "MathFunction.h"
#include "struct.h"
#include "Model.h"
#include "SphereModel.h"
#include "Sprite.h"
#include "DebugCamera.h"
#include "Particle.h"

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

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain (_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	MagosuyaEngine* magosuya = MagosuyaEngine::GetInstance ();
	magosuya->Initialize ();

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
	//テクスチャの読み込み
	TextureManager::GetInstance ()->LoadTexture ("Resources/uvChecker.png", "uvChecker");

	//平行光源のResourceを作成してデフォルト値を書き込む
	ComPtr<ID3D12Resource> dierctionalLightResource = DxCommon::GetInstance()->CreateBufferResource (sizeof (DirectionalLight));
	DirectionalLight* directionalLightData = nullptr;
	//書き込むためのアドレス取得
	dierctionalLightResource->Map (0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	//実際に書き込み
	directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionalLightData->direction = { 0.0f, 0.0f, 1.0f };
	directionalLightData->intensity = 1.0f;
	directionalLightData->mode = Light::halfLambert;
	//ライティング用の変数
	float colorLight[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	//BGM再生
	SoundPlayWave (xAudio2.Get (), soundData1);

	std::unique_ptr<Particle> particle = std::make_unique<Particle> (DxCommon::GetInstance());
	particle->SetTexHandle (TextureManager::GetInstance()->GetTextureHandle ("uvChecker"));
	particle->Initialize ();
	Transform transform = particle->GetTransform ();
	Transform uvTransform = particle->GetUVTransform ();

	//カメラ用
	//Transform
	Transform cameraTransform{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -10.0f} };
	Matrix4x4 cameraMatrix = {};
	Matrix4x4 viewMatrix = {};
	Matrix4x4 projectionMatrix = {};

	//デバッグカメラ
	std::unique_ptr<DebugCamera> debugCamera = std::make_unique<DebugCamera> ();
	debugCamera->Initialize ();
	bool debugMode = false;
	/*********************************/

	/*メインループ！！！！！！！！！*/
	//ウィンドウの×ボタンが押されるまでループ
	while (true) {

		if (WindowsAPI::GetInstance ()->ProcessMessage ()) {
			break;
		}

		//フレーム開始
		magosuya->BeginFrame ();
#ifdef USEIMGUI
		//FPS表示
		ImGui::Begin ("Debug Window");
		ImGui::Text ("FPS: %.1f", ImGui::GetIO ().Framerate);
		ImGui::End ();

		ImGui::Begin ("カメラモード:TAB");
		if (debugMode) {
			ImGui::TextColored (ImVec4 (1, 1, 0, 1), "Current Camera: Debug");
		}
		else if (!debugMode) {
			ImGui::TextColored (ImVec4 (0, 1, 0, 1), "Current Camera: Scene");
		}
		ImGui::End ();
		if (!ImGui::GetIO ().WantCaptureMouse) {
			debugCamera->SetTatchImGui (false);
		}
		else {
			debugCamera->SetTatchImGui (true);
		}

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
		ImGui::End ();
#endif

		//実際のキー入力処理はここ！
		// 押した瞬間だけトグル
		if (InputManager::GetInstance()->GetRawInput ()->Trigger (VK_TAB)) {
			if (!debugMode) {
				debugMode = true;
			}
			else {
				debugMode = false;
			}
		}

		if (InputManager::GetInstance ()->GetRawInput ()->Push ('D')) {
			transform.translate.x += 0.01f;
		}
		if (InputManager::GetInstance ()->GetRawInput ()->Push ('A')) {
			transform.translate.x -= 0.01f;
		}

		//ゲームの処理//
		//=======オブジェクトの更新処理=======//
		//カメラ
		cameraMatrix = MakeAffineMatrix (cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
		viewMatrix = Inverse (cameraMatrix);
		projectionMatrix = MakePerspectiveFOVMatrix (0.45f, float (WindowsAPI::GetInstance ()->kClientWidth) / float (WindowsAPI::GetInstance ()->kClientHeight), 0.1f, 100.0f);

		if (debugMode && !debugCamera->GetTatchImGui ()) {
			debugCamera->Updata (WindowsAPI::GetInstance ()->GetHwnd (), hr, InputManager::GetInstance ());
			viewMatrix = debugCamera->GetViewMatrix ();
			projectionMatrix = debugCamera->GetProjectionMatrix ();
		}

		//vp行列作成
		Matrix4x4 vp = Multiply (viewMatrix, projectionMatrix);
		particle->SetTransform (transform);
		particle->SetUVTransform (uvTransform);
		particle->Update (&vp);

		//光源のdirectionの正規化
		directionalLightData->direction = Normalize (directionalLightData->direction);

		//ImGuiと変数を結び付ける
		// 色変更用のUI
		static float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };  // 初期値：白

		
		//===描画===//
		particle->Draw ();
		//ライティングの設定
		DxCommon::GetInstance ()->GetCommandList ()->SetGraphicsRootConstantBufferView (3, dierctionalLightResource->GetGPUVirtualAddress ());

		//フレーム終了
		magosuya->EndFrame ();
	}
	xAudio2.Reset ();
	SoundUnload (&soundData1);  // バッファ解放
	return 0;
};