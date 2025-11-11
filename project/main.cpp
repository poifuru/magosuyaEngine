#include <Windows.h>
#include "magosuya/MagosuyaEngine.h"
#include "general/function.h"
#include "general/Math.h"
#include "general/struct.h"
#include <format>// C++20のformat() 文字列整形
#include <chrono>	//時間を扱うライブラリ
#include <sstream>// stringstream
#include <memory>
#include <vector>
#include <xaudio2.h>
#pragma comment(lib,"xaudio2.lib")
#include <Xinput.h>
#pragma comment(lib, "xinput.lib")
#include "magosuya/object/3d/Model.h"
#include "magosuya/object/3d/SphereModel.h"
#include "magosuya/object/2d/Sprite.h"
#include "magosuya/utility/camera/DebugCamera.h"
#include "magosuya/utility/Input/InputManager.h"
#include "application/scene/SceneManager.h"

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

	//BGM再生
	SoundPlayWave (xAudio2.Get (), soundData1);

	std::unique_ptr<SceneManager> sceneManager = std::make_unique<SceneManager> (magosuya.get());
	sceneManager->Initialize (SceneLabel::Title);

	//平行光源のResourceを作成してデフォルト値を書き込む
	ComPtr<ID3D12Resource> dierctionalLightResource = magosuya->GetDxCommon ()->CreateBufferResource (sizeof (DirectionalLight));
	DirectionalLight* directionalLightData = nullptr;
	//書き込むためのアドレス取得
	dierctionalLightResource->Map (0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	//実際に書き込み
	directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionalLightData->direction = { -0.75f, -0.75f, 0.75f };
	directionalLightData->intensity = 1.0f;
	directionalLightData->mode = Light::halfLambert;
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

		//光源のdirectionの正規化
		directionalLightData->direction = Normalize (directionalLightData->direction);

		sceneManager->Update ();

		//***描画処理***//
		//ライティングの設定
		magosuya->GetDxCommon ()->GetCommandList ()->SetGraphicsRootConstantBufferView (3, dierctionalLightResource->GetGPUVirtualAddress ());
		sceneManager->Draw ();
		//*************//

		//フレーム終了
		g_inputManager->EndFrame ();
		magosuya->EndFrame ();
	}

	xAudio2.Reset ();
	SoundUnload (&soundData1);  // バッファ解放
	magosuya->Finalize ();
	return 0;
};